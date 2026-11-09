#pragma once

#include "addr.h"
#include "msg_queue.h"
#include "net_types.h"
#include "link_awaiter.h"
#include "task.h"

#include "proto/ip_packet.pb.h"
#include "proto/icmp_packet.pb.h"
#include "proto/tcp_packet.pb.h"
#include "proto/udp_packet.pb.h"
#include "proto/dataframe.pb.h"

#include <string>
#include <expected>
#include <stdexcept>
#include <unordered_map>
#include <system_error>
#include <tuple>

class OS;
class NIC;
class File;
class Internet;
struct NetNode;

class NetManager
{
public:

	NetManager() = delete;
	NetManager(OS* owner);
	NetManager(NetManager&) = delete;
	NetManager(NetManager&&) = delete;
	~NetManager();

	std::expected<OpenSocketPair, std::error_condition> create_socket(SocketType type = SocketType::Stream);
	std::error_condition close_socket(OpenSocketHandle h);
	std::error_condition close_socket(OpenSocketEntry* sock);
	Task<std::error_condition> async_close_socket(OpenSocketHandle h);

	std::error_condition bind_socket(OpenSocketHandle sock, AddressPair addr);
	std::error_condition bind_socket(OpenSocketHandle sock, Address6 addr, int32_t port);

	Task<std::error_condition> async_connect_socket(OpenSocketHandle sock, AddressPair addr);
	Task<std::error_condition> async_connect_socket(OpenSocketHandle sock, Address6 addr, int32_t port);
	
	Task<std::expected<OpenSocketPair, std::error_condition>> async_accept_socket(OpenSocketHandle sock);

	Task<NetReadResult> async_read_socket(OpenSocketHandle sock);
	Task<NetReadResultTcp> async_read_socket_tcp(OpenSocketHandle sock);
	Task<NetReadResultIp> async_read_socket_raw(OpenSocketHandle sock);

	Task<size_t> async_write_socket(OpenSocketHandle sock, std::string bytes);
	Task<size_t> async_write_socket_raw(OpenSocketHandle sock, ip::IpPackage&& package);

	Task<bool> async_socket_test_alive(OpenSocketHandle sock);
	
	int32_t listen(OpenSocketHandle sock);

	void route(ip::IpPackage&& package);
	void arp_buffer(ip::IpPackage&& package);
	
	void send(ip::IpPackage&& package);
	void send(ip::DataFrame&& frame);
	void send(ip::IpPackage&& package, Uid64 mac);

	void unpack(ip::IpPackage&& package);
	void unpack(ip::IpPackage&& package, const Address6& src_addr, const Address6& dest_addr);

	NetFrameAwaiter async_read_rx();
	NetMessageAwaiter async_read_tx();
	NetMessageAwaiter async_read_route();
	NetMessageAwaiter async_read_arp();

	Address6 get_primary_ip() const;
	Uid64 get_mac() const;

	bool socket_is_open(OpenSocketHandle h) const;
	bool socket_has_data(OpenSocketHandle h) const;

	bool create_session(OpenSocketHandle h);

	std::optional<Uid64> arp_lookup(Address6 addr);
	void arp_refresh(Uid64 mac, Address6 addr);

protected:

	void handle_packet(ip::IcmpPacket&& packet, ip::IpPackage&& outer, const Address6& src_addr, const Address6& dest_addr);
	void handle_packet(ip::TcpPacket&& packet, ip::IpPackage&& outer, const Address6& src_addr, const Address6& dest_addr);
	void handle_packet(ip::UdpPacket&& packet, ip::IpPackage&& outer, const Address6& src_addr, const Address6& dest_addr);
	
	OpenSocketEntry* find_socket(OpenSocketHandle sock_fd);
	OpenSocketEntry* find_socket(const AddressPair& tuple);
	OpenSocketEntry* find_socket(const AddressTuple& tuple);

	std::optional<ip::TcpPacket> make_tcp_query(OpenSocketHandle h, ip::TcpType type, std::string&& payload);
	std::optional<ip::IpPackage> make_tcp_query_ip(OpenSocketHandle h, ip::TcpType type, std::string&& payload);

	std::optional<ip::TcpPacket> make_tcp_reply(OpenSocketHandle h, ip::TcpType type, std::string&& payload);
	std::optional<ip::IpPackage> make_tcp_reply_ip(OpenSocketHandle h, ip::TcpType type, std::string&& payload);

	OpenSocketHandle get_handle();
	void return_handle(OpenSocketHandle h);

protected:
	
	OS& os_;
	Internet& internet_;

	std::shared_ptr<NetNode> node_{nullptr};

	uint64_t handle_counter_{1};
	std::set<OpenSocketHandle> free_handles_{};

	Address6 address_{};
	
	NetQueue rx_queue_{};
	NetQueue tx_queue_{};
	NetQueue routing_queue_{};
	NetQueue arp_queue_{};

	std::unordered_map<OpenSocketHandle, OpenSocketEntry> sockets_;
	std::unordered_map<Address6, Uid64> arp_cache_;
	std::vector<OpenSocketHandle> raw_sockets_;

	friend class ProcNetApi;
};