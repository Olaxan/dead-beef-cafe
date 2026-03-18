#pragma once

#include "addr.h"
#include "file_socket.h"
#include "msg_queue.h"
#include "net_types.h"
#include "link_awaiter.h"
#include "task.h"

#include "proto/ip_packet.pb.h"
#include "proto/icmp_packet.pb.h"
#include "proto/tcp_packet.pb.h"
#include "proto/udp_packet.pb.h"

#include <string>
#include <expected>
#include <stdexcept>
#include <unordered_map>
#include <system_error>
#include <tuple>

class OS;
class NIC;
class File;

class NetManager
{
public:

	NetManager() = delete;
	NetManager(OS* owner);
	NetManager(NetManager&) = delete;
	NetManager(NetManager&&) = delete;
	~NetManager();

	std::expected<OpenSocketPair, std::error_condition> create_socket();
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
	
	int32_t listen(OpenSocketHandle sock);

	void route(ip::IpPackage&& package);

	void safe_rx(ip::IpPackage&& package);

	void send(ip::IpPackage&& package);
	void send(ip::IpPackage&& package, Uid64 mac);

	void receive(ip::IpPackage&& package);
	void receive(ip::IpPackage&& package, const Address6& src_addr, const Address6& dest_addr);

	NetMessageAwaiter async_read_rx();
	NetMessageAwaiter async_read_tx();
	NetMessageAwaiter async_read_route();

	Address6 get_primary_ip() const;
	bool socket_is_open(OpenSocketHandle h) const;
	bool socket_has_data(OpenSocketHandle h) const;

	bool create_session(OpenSocketHandle h);

	void arp_request();
	void arp_request(Uid64 mac);
	std::optional<Uid64> arp_lookup(Address6 addr);
	
	LinkUpdateAwaiter async_await_link();

	void link_unicast(Uid64 mac, NetCastFn unicast_fn);
	void link_broadcast(NetCastFn unicast_fn);

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
	
	OS* os_;
	NIC* nic_{nullptr};

	uint64_t handle_counter_{1};
	std::set<OpenSocketHandle> free_handles_{};

	NetQueue routing_queue_{};

	std::unordered_map<OpenSocketHandle, OpenSocketEntry> sockets_;
	std::unordered_map<Address6, Uid64> arp_cache_;

	friend class ProcNetApi;
};