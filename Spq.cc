#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ppp-header.h"
#include "Spq.h"


#include <math.h>
#include <queue>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using namespace ns3;

//NS_LOG_COMPONENT_DEFINE("DiffServ");


NS_LOG_COMPONENT_DEFINE("Spq");

//NS_OBJECT_TEMPLATE_CLASS_DEFINE(DiffServ, Packet);

template <typename Packet>
TypeId
DiffServ<Packet>::GetTypeId(void)
{
    static TypeId tid = TypeId("DiffServ<Packet>")
                            .SetParent<Queue<Packet>>()
                            .SetGroupName("DiffServ")
                            .template AddConstructor<DiffServ<Packet>>();
    return tid;
}

template <typename Packet>
DiffServ<Packet>::DiffServ()
    // : Queue<Packet>()
{
    NS_LOG_FUNCTION(this);
    in = 0;
    out = 0;
}

template <typename Packet>
DiffServ<Packet>::DiffServ(uint32_t maxPackets,
                           uint32_t priority_level,
                           bool isDefault,
                           Ipv4Address destIpAddr,
                           Ipv4Mask destMask,
                           uint32_t destPortNum,
                           uint32_t protNum,
                           Ipv4Address sourIpAddr,
                           Ipv4Mask sourMask,
                           uint32_t sourPortNum,
                           Ipv4Address sourIp,
                           Ipv4Address destIp)
    // : Queue<Packet>()
{
    NS_LOG_FUNCTION(this);

    TrafficClass* tc = new TrafficClass(maxPackets,
                                        priority_level,
                                        isDefault,
                                        destIpAddr,
                                        destMask,
                                        destPortNum,
                                        protNum,
                                        sourIpAddr,
                                        sourMask,
                                        sourPortNum,
                                        sourIp,
                                        destIp);
    q_class.push_back(tc);
    in = 0;
    out = 0;
}

template <typename Packet>
DiffServ<Packet>::~DiffServ()
{
    NS_LOG_FUNCTION(this);
}

template <typename Packet>
bool
DiffServ<Packet>::DoEnqueue(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    bool res = Classify(p);
    in++;
    return res;
}

template <typename Packet>
Ptr<Packet>
DiffServ<Packet>::DoDequeue()
{
    NS_LOG_FUNCTION(this);
    if (in <= out)
    {
        return NULL;
    }
    Ptr<Packet> res = Schedule()->Dequeue();
    out++;
    return res;
}

template <typename Packet>
Ptr<Packet>
DiffServ<Packet>::DoRemove()
{
    NS_LOG_FUNCTION(this);
    if (in <= out)
    {
        return NULL;
    }
    Ptr<Packet> res = Schedule()->Dequeue();
    out++;
    return res;
}

template <typename Packet>
Ptr<const Packet>
DiffServ<Packet>::DoPeek()
{
    NS_LOG_FUNCTION(this);
    return q_class[out % q_class.size()]->Peek();
}

template <typename Packet>
Ptr<Packet>
DiffServ<Packet>::Schedule()
{
    return q_class[out % q_class.size()]->Dequeue();
}

template <typename Packet>
uint32_t
DiffServ<Packet>::Classify(Ptr<Packet> p)
{
   return q_class[in % q_class.size()]->Enqueue(p);
}

template <typename Packet>
SPQ<Packet>::SPQ()
{
    NS_LOG_FUNCTION (this);
}

template <typename Packet>
SPQ<Packet>::SPQ(std::vector<MyConfig> configs)
{
    NS_LOG_FUNCTION (this);
    q_num = 0;
    for (uint32_t i = 0; i < configs.size(); i++)
    {
        MyConfig config = configs[i];
        TrafficClass* tc = new TrafficClass(config.maxPackets,
                                         config.priority_level,
                                         config.isDefault,
                                         config.destIpAddr,
                                         config.destMask,
                                         config.destPortNum,
                                         config.protNum,
                                         config.sourIpAddr,
                                         config.sourMask,
                                         config.sourPortNum,
                                         config.sourceIp,
                                         config.destIp);
        q_class.push_back(tc);
        q_num++;
    }
}

template <typename Packet>
SPQ<Packet>::~SPQ()
{
    NS_LOG_FUNCTION (this);
}

template <typename Packet>
TypeId
SPQ<Packet>::GetTypeId()
{
    static TypeId tid = TypeId("SPQ<Packet>")
                            .SetParent<Queue<Packet>>()
                            .SetGroupName("SPQ")
                            .template AddConstructor<SPQ<Packet>>();
    return tid;
}

template <typename Packet>
bool
SPQ<Packet>::Enqueue(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    return DoEnqueue(packet);
}

template <typename Packet>
Ptr<Packet>
SPQ<Packet>::Dequeue()
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> packet = DoDequeue();

    NS_LOG_LOGIC("Popped " << packet);

    return packet;
}

template <typename Packet>
Ptr<Packet>
SPQ<Packet>::Remove()
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> packet = DoRemove();

    NS_LOG_LOGIC("Removed " << packet);

    return packet;
}

template <typename Packet>
Ptr<const Packet>
SPQ<Packet>::Peek() const
{
    NS_LOG_FUNCTION(this);

    return DoPeek();
}

template <typename Packet>
bool
SPQ<Packet>::DoEnqueue(Ptr<Packet> packet)
{
    uint32_t index_of_qclass = Classify(packet);
    if (index_of_qclass >= 0 && index_of_qclass < q_num)
    {
        if (q_class[index_of_qclass]->GetPackets() < q_class[index_of_qclass]->GetMaxPackets())
        {
            q_class[index_of_qclass]->getMqueue()->push(packet);
            return true;
        }
    }
    else
    {
        for (TrafficClass* tc : q_class)
        {
            if (tc->GetDefault() == true)
            {
                if (tc->GetPackets() < tc->GetMaxPackets())
                {
                    tc->getMqueue()->push(packet);
                    return true;
                }
            }
        }
    }
    return false;
}

template <typename Packet>
Ptr<Packet>
SPQ<Packet>::DoDequeue()
{
    std::queue<Ptr<Packet>>* mqueue = Schedule();
    if(mqueue != nullptr && !mqueue->empty()) {
        Ptr<Packet> packet = mqueue->front();
        mqueue->pop();
        return packet;
    } 
    return nullptr;
}

template <typename Packet>
Ptr<Packet>
SPQ<Packet>::DoRemove()
{
    std::queue<Ptr<Packet>>* mqueue = Schedule();
    if(mqueue->size() != 0) {
        Ptr<Packet> packet = mqueue->front();
        mqueue->pop();
        return packet;
    } 
    return nullptr;
}

template <typename Packet>
Ptr<const Packet>
SPQ<Packet>::DoPeek() const
{
    for(uint32_t i = 0; i < q_num; i++) {
        if(!q_class[i]->IsEmpty()) {
            return q_class[i]->getMqueue()->front();
        }
    }
    return nullptr;
}

template <typename Packet>
uint32_t
SPQ<Packet>::Classify(Ptr<Packet> p)
{
    for (uint32_t i = 0; i < q_num; i++)
    {
        if (q_class[i]->match(p))
        {
            return i;
        }
    }
    return -1;
}

template <typename Packet>
std::queue<Ptr<Packet>>*
SPQ<Packet>::Schedule()
{
    for(uint32_t i = 0; i < q_num; i++) {
        if(!q_class[i]->IsEmpty()) {
            return q_class[i]->getMqueue();
        }
    }
    return nullptr;
}

int
main(int argc, char* argv[])
{
    // config file path
    std::string fileName = "";

    CommandLine cmd(__FILE__);
    cmd.AddValue("spq", "file name", fileName);
    cmd.Parse(argc, argv);

    // read config
    std::vector<MyConfig> myConfigs;

    std::ifstream configFile(fileName);
    if (!configFile.is_open()) {
        std::cerr << "Failed to open config file: " << fileName << std::endl;
        return 1;
    }

    std::string line;

    MyConfig currentConfig = {};
    bool isNewConfig = true;

    while (getline(configFile, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            std::cout << "\n" << std::endl;
            continue;
        }

        if (isNewConfig) {
            if (startsWith(line, "\"queueId\"")) {
                isNewConfig = false;
                currentConfig = {};
            }
        } else {
            setConfigValue(line, currentConfig);
            if (startsWith(line, "\"destIP\"")) {
                myConfigs.push_back(currentConfig);
                isNewConfig = true;
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        MyConfig c = myConfigs[i];
        std::cout << "maxPackets = " << c.maxPackets << std::endl;
        std::cout << "priorityLevel = " << c.priority_level << std::endl;
        std::cout << "isDefault = " << c.isDefault << std::endl;
        std::cout << "sourceIpAddress = " << c.sourIpAddr << std::endl;
        std::cout << "sourceMask = " << c.sourMask << std::endl;
        std::cout << "sourcePortNumber = " << c.sourPortNum << std::endl;
        std::cout << "destIpAddress = " << c.destIpAddr << std::endl;
        std::cout << "destMask = " << c.destMask << std::endl;
        std::cout << "destPortNumber = " << c.destPortNum << std::endl;
        std::cout << "protocolNumber = " << c.protNum << std::endl;
        std::cout << "sourceIP = " << c.sourceIp << std::endl;
        std::cout << "destIP = " << c.destIp << std::endl;
        std::cout << "\n"  << std::endl;
    }

    // LogComponentEnable("UdpApplication", LOG_LEVEL_INFO);
    // LogComponentEnable("UdpServerApplication", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create(3);

    ns3::NodeContainer n0r = NodeContainer(nodes.Get(0), nodes.Get(1));
    ns3::NodeContainer n1r = NodeContainer(nodes.Get(1), nodes.Get(2));

    InternetStackHelper internet;
    internet.Install(nodes);

    NetDeviceContainer d0r;
    Ptr<PointToPointNetDevice> devA = CreateObject<PointToPointNetDevice>();
    devA->SetAttribute("DataRate", StringValue("4Mbps"));
    devA->SetAddress(Mac48Address::Allocate());
    n0r.Get(0)->AddDevice(devA);
    Ptr<SPQ<Packet>> queueA = CreateObject<SPQ<Packet>>(myConfigs);
    devA->SetQueue(queueA);

    Ptr<PointToPointNetDevice> devB = CreateObject<PointToPointNetDevice>();
    devB->SetAttribute("DataRate", StringValue("4Mbps"));
    devB->SetAddress(Mac48Address::Allocate());
    n0r.Get(1)->AddDevice(devB);
    Ptr<SPQ<Packet>> queueB = CreateObject<SPQ<Packet>>(myConfigs);
    devB->SetQueue(queueB);

    Ptr<PointToPointChannel> channel1 = CreateObject<PointToPointChannel>();
    devA->Attach(channel1);
    devB->Attach(channel1);
    d0r.Add(devA);
    d0r.Add(devB);

    NetDeviceContainer d1r;
    Ptr<PointToPointNetDevice> devC = CreateObject<PointToPointNetDevice>();
    devC->SetAttribute("DataRate", StringValue("1Mbps"));
    devC->SetAddress(Mac48Address::Allocate());
    n1r.Get(0)->AddDevice(devC);
    Ptr<SPQ<Packet>> queueC = CreateObject<SPQ<Packet>>(myConfigs);
    devC->SetQueue(queueC);

    Ptr<PointToPointNetDevice> devD = CreateObject<PointToPointNetDevice>();
    devD->SetAttribute("DataRate", StringValue("1Mbps"));
    devD->SetAddress(Mac48Address::Allocate());
    n1r.Get(1)->AddDevice(devD);
    Ptr<SPQ<Packet>> queueD = CreateObject<SPQ<Packet>>(myConfigs);
    devD->SetQueue(queueD);

    Ptr<PointToPointChannel> channel2 = CreateObject<PointToPointChannel>();
    devC->Attach(channel2);
    devD->Attach(channel2);
    d1r.Add(devC);
    d1r.Add(devD);

    // eNS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i0r = ipv4.Assign(d0r);
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i1r = ipv4.Assign(d1r);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    UdpServerHelper Server(9);

    ApplicationContainer serverApps = Server.Install(nodes.Get(2));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    Ptr<Socket> socket1 = Socket::CreateSocket(n0r.Get(0), UdpSocketFactory::GetTypeId());
    socket1->Bind();
    Address address1(InetSocketAddress(i1r.GetAddress(1), 9));

    Ptr<Socket> socket2 = Socket::CreateSocket(n0r.Get(0), UdpSocketFactory::GetTypeId());
    socket2->Bind();
    Address address2(InetSocketAddress(i1r.GetAddress(1), 9));

    Ptr<UdpApplication> clientApps1 = CreateObject<UdpApplication>();
    nodes.Get(0)->AddApplication(clientApps1);
    clientApps1->SetStartTime(Seconds(1.0));
    clientApps1->SetStopTime(Seconds(20.0));
    clientApps1->Setup(socket1, address1, 1000, 1000000, DataRate("4Mbps"), 8888);

    Ptr<UdpApplication> clientApps2 = CreateObject<UdpApplication>();
    nodes.Get(0)->AddApplication(clientApps2);
    clientApps2->SetStartTime(Seconds(10.0));
    clientApps2->SetStopTime(Seconds(20.0));
    clientApps2->Setup(socket2, address2, 1000, 1500000, DataRate("4Mbps"), 9999);

    PointToPointHelper pointToPoint0;
    pointToPoint0.EnablePcapAll("udp");

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}