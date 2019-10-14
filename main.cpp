#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>
#include <pthread.h>

#include <string>

#include "bytebuf.h"
#include "ieee80211_radiotap.h"
#include "ieee80211.h"
#include "controller_data.h"


constexpr uint8_t broadcastAddress[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff 
};



class WifiRawSocket {
// some pieces were extracted from @medusalix' implementation
public:
    int initialize(const std::string& device_name) {
        // open raw socket
        sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if(sock < 0) {
            perror("failed opening raw socket");
            return -1;
        }

        // get interface index
        struct ifreq if_idx;
        memset(&if_idx, 0, sizeof(struct ifreq));
        strncpy(if_idx.ifr_name, device_name.c_str(), IFNAMSIZ-1);
        if (ioctl(sock, SIOCGIFINDEX, &if_idx) < 0) {
            perror("SIOCGIFINDEX");
            return -1;
        }

        // get interface MAC address
        struct ifreq if_mac;
        memset(&if_mac, 0, sizeof(struct ifreq));
        strncpy(if_mac.ifr_name, device_name.c_str(), IFNAMSIZ-1);
        if (ioctl(sock, SIOCGIFHWADDR, &if_mac) < 0) {
            perror("SIOCGIFHWADDR");
            return -1;
        }
        memcpy(mac, if_mac.ifr_hwaddr.sa_data, 6);

        // bind raw socket to interface
        struct sockaddr_ll sa;
        memset(&sa, 0, sizeof(sa));
        sa.sll_family = AF_PACKET;
        sa.sll_ifindex = if_idx.ifr_ifindex;
        if (bind(sock, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
            perror("bind");
            return -1;
        }

        return 0;
    }

    int sendBeacon(bool pairing) {
        uint8_t variable[] = {
            0xdd, 0x10,    0x00, 0x50, 0xf2, 0x11, 0x01,
            0x10, pairing, 0xa1, 0x28, 0x9d, 0x00, 0x00,
            0x00, 0x00,    0x00, 0x00
        };

        auto frame = preparePacket(FT_MGMT, FT_BEACON, broadcastAddress);
        BeaconFrame beacon = {
            .timestamp      = (uint64_t) seq * 102400,
            .interval       = 0x64,
            .capabilityInfo = 0xc631,
            .ssid           = 0
        };

        return sendPacket(frame, beacon, variable, sizeof(variable));
    }

    void processPacket() {
        uint8_t buf[2048];
        uint16_t off = 0;
        size_t ret = read(sock, buf, sizeof(buf));
        if (ret < 0) {
            perror("read");
        }

        // strip radiotap header
        struct ieee80211_radiotap_header* hdr = (struct ieee80211_radiotap_header*) buf;

        auto frame = (struct WlanFrame*) (buf + hdr->it_len);
        if (memcmp(frame->destination, mac, sizeof(mac)) != 0) {
            return;
        }

        switch(frame->frameControl.type) {
        case FT_MGMT:
            switch(frame->frameControl.subtype) {
            case FT_PAIR:
                {
                    struct PairingFrame *pairing = (PairingFrame*) (frame + sizeof(frame));
                    handlePairingRequest(frame->source, pairing);
                }
                break;
            case FT_ASSOC_REQ:
                handleAssociationRequest(frame->source, true);
                break;
            case FT_REASSOC_REQ:
                handleAssociationRequest(frame->source, false);
                break;
            case FT_PROBE_REQ:
                handleProbeRequest(frame->source);
                break;
            }
            break;
        case FT_DATA:
            switch(frame->frameControl.subtype) {
            case FT_QOS_DATA:
                {
                    auto &controllerFrame = * (struct ControllerFrame*) (buf + hdr->it_len + sizeof(WlanFrame) + sizeof(QosFrame));
                    handleData(frame->source, controllerFrame);
                }
                break;
            }
        }
    }

private:
    template<typename T> int sendPacket(struct WlanFrame& frame, T& subFrame, uint8_t *payload = nullptr, uint16_t len = 0) {
        ByteBuf buf;

        struct ieee80211_radiotap_header hdr = {
            .it_version = 0,
            .it_pad     = 0,
            .it_len     = sizeof(hdr) + 4,
            .it_present = 1 << IEEE80211_RADIOTAP_CHANNEL
        };
        buf.appendBytes(&hdr, sizeof(hdr));

        uint32_t channel = (IEEE80211_CHAN_OFDM | IEEE80211_CHAN_5GHZ) << 16 | 5200;
        buf.appendBytes(&channel, sizeof(channel));

        buf.appendBytes(&frame, sizeof(frame));
        buf.appendBytes(&subFrame, sizeof(subFrame));
        buf.appendBytes(payload, len);

        int ret = write(sock, buf.buf(), buf.len());
        if (ret < 0) {
            perror("write");
            return ret;
        } else if (ret < buf.len()) {
            printf("failed to write complete packet\n");
            return -1;
        }
        return 0;
    }

    struct WlanFrame preparePacket(uint8_t type, uint8_t subtype, const uint8_t destination[6]) {
        struct WlanFrame ret = {
            .frameControl = {
                .protocolVersion = 0,
                .type            = type,
                .subtype         = subtype,
            },
            .duration = 0,
            .sequenceControl = (uint16_t) (seq++ << 4)
        };
        memcpy(&ret.destination, destination, 6);
        memcpy(&ret.source,      mac, 6);
        memcpy(&ret.bssId,       mac, 6);
        return ret;
    }

    void handlePairingRequest(uint8_t controllerMac[6], PairingFrame *pairingRequest) {
        //printf("Got a pairing request\n");

        //printf("Sending pairing response\n");
        auto frame = preparePacket(FT_MGMT, FT_PAIR, controllerMac);
        struct PairingFrame pairingResponse = {
            .unknown = 0x07,
            .type    = PAIR_RESPONSE
        };
        uint8_t buf = 0;
        sendPacket(frame, pairingResponse, &buf, 1);
    }

    void handleAssociationRequest(uint8_t controllerMac[6], bool doHandshake) {
        //printf("Got an association request\n");

        //printf("Sending association response\n");
        auto frame = preparePacket(FT_MGMT, FT_ASSOC_RESP, controllerMac);
        struct AssociationResponseFrame resp = {};
        sendPacket(frame, resp);

        if (doHandshake) {
            printf("Executing handshake\n");
            uint8_t
                frame2Data[] = { 0x00 },
                frame3Data[] = { 0x04 };
            struct LedModeData ledData = {
                .mode       = 0x01,
                .brightness = 0x14
            };
            struct ControllerFrame frame1 = {
                .command = 0x01,
                .message = 0x20
            }, frame2 = {
                .command = 0x05,
                .message = 0x20,
                .length  = sizeof(frame2Data)
            }, frame3 = {
                .command = 0x1e,
                .message = 0x30,
                .length  = sizeof(frame3Data)
            }, frame4 = {
                .command = 0x0a,
                .message = 0x20,
                .length  = sizeof(ledData)
            };

            sendControllerPacket(controllerMac, frame1);
            sendControllerPacket(controllerMac, frame2, frame2Data, sizeof(frame2Data));
            sendControllerPacket(controllerMac, frame3, frame3Data, sizeof(frame3Data));
            sendControllerPacket(controllerMac, frame4, (uint8_t*) &ledData, sizeof(ledData));
        }
    }

    void handleData(uint8_t controllerMac[6], struct ControllerFrame &controllerFrame) {
        //printf("data packet from controller\n");

        if (controllerFrame.command == 0x1e && controllerFrame.message == 0x20 && controllerFrame.length == 0x10) {
            //printf("received serial, starting controller\n");
            struct ControllerFrame resp = {
                .command = 0x01,
                .message = 0x20,
                .sequence = controllerFrame.sequence,
                .length = 0x09
            };
            uint8_t data[] = { 0x00, 0x1e, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 };
            sendControllerPacket(controllerMac, resp, data, sizeof(data));
        } else if (controllerFrame.command == 0x20 && controllerFrame.message == 0x00 && controllerFrame.length == 0x0e) {
            auto &data = * (struct InputData*) ((uint8_t*) &controllerFrame + sizeof(controllerFrame));
            if (memcmp(&data, &lastInputData, sizeof(data)) == 0) {
                // input data didn't change
                return;
            }
            memcpy(&lastInputData, &data, sizeof(data));

            auto &buttons = data.buttons;
            printf("A: %d B: %d X: %d Y: %d st: %d ba: %d ↑: %d ↓: %d ←: %d →: %d b←: %d b→: %d stick←: %d stick→: %d tr←: %d tr→: %d stick←X: %d stick←Y: %d stick→X: %d stick→Y: %d\n", buttons.a, buttons.b, buttons.x, buttons.y, buttons.start, buttons.back, buttons.dpadUp, buttons.dpadDown, buttons.dpadLeft, buttons.dpadRight, buttons.bumperLeft, buttons.bumperRight, buttons.stickLeft, buttons.stickRight, data.triggerLeft, data.triggerRight, data.stickLeftX, data.stickLeftY, data.stickRightX, data.stickRightY);
        }
    }

    void sendControllerPacket(uint8_t controllerMac[6], struct ControllerFrame &controllerFrame, uint8_t* payload = nullptr, uint16_t len = 0) {
        auto frame = preparePacket(FT_DATA, FT_QOS_DATA, controllerMac);
        frame.frameControl.fromDs = 1;
        frame.duration = 144;
        struct QosFrame qos = {};
        ByteBuf buf;
        buf.appendBytes(&controllerFrame, sizeof(controllerFrame));
        buf.appendBytes(payload, len);
        sendPacket(frame, qos, buf.buf(), buf.len());
    }

    int handleProbeRequest(uint8_t controllerMac[6]) {
        //printf("Got probe request\n");

        //printf("Sending probe response\n");
        uint8_t variable[] = {
            0xdd, 0x10, 0x00, 0x50, 0xf2, 0x11, 0x01,
            0x10, 0x00, 0xa1, 0x28, 0x9d, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00
        };

        auto frame = preparePacket(FT_MGMT, FT_PROBE_RESP, controllerMac);
        BeaconFrame beacon = {
            .timestamp      = (uint64_t) seq * 102400,
            .interval       = 0x64,
            .capabilityInfo = 0xc631,
            .ssid           = 0
        };

        return sendPacket(frame, beacon, variable, sizeof(variable));
    }

private:
    struct InputData lastInputData;
    uint16_t seq;
    int sock;
    uint8_t mac[6];
};

WifiRawSocket s;

void* send_beacons(void*) {
    while(true) {
        s.sendBeacon(true);
        usleep(102400);
    }

    return nullptr;
}


int main(int argc, char **argv) {
    if(s.initialize(argv[1]) < 0) {
        return 1;
    }
    pthread_t beacon_thread;
    pthread_create(&beacon_thread, nullptr, send_beacons, nullptr);

    while(true) {
        s.processPacket();
    }
    return 0;
}
