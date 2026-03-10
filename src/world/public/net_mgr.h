#pragma once

#include "addr.h"
#include "file_socket.h"
#include "proc_io.h"
#include "msg_queue.h"
#include "netw.h"
#include "task.h"

#include "proto/ip_packet.pb.h"
#include "proto/icmp_packet.pb.h"
#include "proto/tcp_packet.pb.h"
#include "proto/udp_packet.pb.h"

#include <string>
#include <expected>
#include <stdexcept>
#include <unordered_map>

class OS;
class NIC;
class File;

class NetManager
{
public:

	NetManager() = delete;
	NetManager(OS* owner);
	~NetManager();

	std::expected<SocketDescriptor, std::runtime_error> create_socket();

	int32_t bind_socket(SocketDescriptor sock, AddressPair addr);
	int32_t bind_socket(SocketDescriptor sock, Address6 addr, int32_t port);

	Task<int32_t> async_connect_socket(SocketDescriptor sock, AddressPair addr);
	Task<int32_t> async_connect_socket(SocketDescriptor sock, Address6 addr, int32_t port);
	
	Task<SocketDescriptor> async_accept_socket(SocketDescriptor sock);

	Task<std::string> async_read_socket(SocketDescriptor sock);
	Task<ip::TcpPacket> async_read_socket_tcp(SocketDescriptor sock);
	Task<ip::IpPackage> async_read_socket_raw(SocketDescriptor sock);

	Task<size_t> async_write_socket(SocketDescriptor sock, std::string bytes);
	
	int32_t listen(SocketDescriptor sock);

	void add_socket_filter(SocketFilter&& filter);
	void remove_socket_filter(void* listener);

	void send(ip::IpPackage&& package);
	void send(ip::IpPackage&& package, Uid64 mac);

	void receive(ip::IpPackage&& package);
	void receive(ip::IpPackage&& package, const Address6& src_addr, const Address6& dest_addr);

	void handle_packet(ip::IcmpPacket&& packet, ip::IpPackage&& outer, const Address6& src_addr, const Address6& dest_addr);
	void handle_packet(ip::TcpPacket&& packet, ip::IpPackage&& outer, const Address6& src_addr, const Address6& dest_addr);
	void handle_packet(ip::UdpPacket&& packet, ip::IpPackage&& outer, const Address6& src_addr, const Address6& dest_addr);

	NetMessageAwaiter async_read_rx();
	NetMessageAwaiter async_read_tx();

	Address6 get_primary_ip() const;

	bool socket_is_open(SocketDescriptor fd) const;

	bool create_session(SocketDescriptor fd);

	LinkUpdateAwaiter async_await_link();
	void arp_request();
	void arp_request(Uid64 mac);
	std::optional<Uid64> arp_lookup(Address6 addr);

protected:

	SocketFile* find_socket(SocketDescriptor sock_fd);
	SocketFile* find_socket(const AddressPair& tuple);
	SocketFile* find_socket(const AddressTuple& tuple);
	void broadcast_socket_rx(SocketDescriptor fd, const std::string& payload);

	OS& os_;
	NIC* nic_{nullptr};

	uint64_t socket_index_{1};

	std::unordered_map<SocketDescriptor, std::shared_ptr<SocketFile>> sockets_;
	std::unordered_map<AddressTuple, SocketDescriptor> sessions_;
	std::unordered_map<AddressPair, SocketDescriptor> bindings_;
	std::unordered_map<Address6, Uid64> arp_cache_;
	std::vector<SocketFilter> socket_filters_;

};