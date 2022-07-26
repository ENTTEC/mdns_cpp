#include "mdns_cpp/utils.hpp"

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <errno.h>
#include <stdio.h>

#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <winsock.h>
#warning "Windows is not supported yet to send mac address"
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <printf.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <array>
#include <cstdio>
#include <regex>
#include <string>
#endif  // _WIN32

#include "mdns_cpp/macros.hpp"

namespace mdns_cpp {

std::string getHostName() {
  const char *hostname = "dummy-host";

#ifdef _WIN32
  WORD versionWanted = MAKEWORD(1, 1);
  WSADATA wsaData;
  if (WSAStartup(versionWanted, &wsaData)) {
    const auto msg = "Error: Failed to initialize WinSock";
    MDNS_LOG << msg << "\n";
    throw std::runtime_error(msg);
  }

  char hostname_buffer[256];
  DWORD hostname_size = (DWORD)sizeof(hostname_buffer);
  if (GetComputerNameA(hostname_buffer, &hostname_size)) {
    hostname = hostname_buffer;
  }

#else

  char hostname_buffer[256];
  const size_t hostname_size = sizeof(hostname_buffer);
  if (gethostname(hostname_buffer, hostname_size) == 0) {
    hostname = hostname_buffer;
  }

#endif

  return hostname;
}

std::string ipv4AddressToString(char *buffer, size_t capacity, const sockaddr_in *addr, size_t addrlen) {
  char host[NI_MAXHOST] = {0};
  char service[NI_MAXSERV] = {0};
  const int ret = getnameinfo((const struct sockaddr *)addr, (socklen_t)addrlen, host, NI_MAXHOST, service, NI_MAXSERV,
                              NI_NUMERICSERV | NI_NUMERICHOST);
  int len = 0;
  if (ret == 0) {
    if (addr->sin_port != 0) {
      len = snprintf(buffer, capacity, "%s:%s", host, service);
    } else {
      len = snprintf(buffer, capacity, "%s", host);
    }
  }
  if (len >= (int)capacity) {
    len = (int)capacity - 1;
  }

  return std::string(buffer, len);
}

std::string ipv6AddressToString(char *buffer, size_t capacity, const sockaddr_in6 *addr, size_t addrlen) {
  char host[NI_MAXHOST] = {0};
  char service[NI_MAXSERV] = {0};
  const int ret = getnameinfo((const struct sockaddr *)addr, (socklen_t)addrlen, host, NI_MAXHOST, service, NI_MAXSERV,
                              NI_NUMERICSERV | NI_NUMERICHOST);
  int len = 0;
  if (ret == 0) {
    if (addr->sin6_port != 0) {
      { len = snprintf(buffer, capacity, "[%s]:%s", host, service); }
    } else {
      len = snprintf(buffer, capacity, "%s", host);
    }
  }
  if (len >= (int)capacity) {
    len = (int)capacity - 1;
  }

  return std::string(buffer, len);
}

std::string ipAddressToString(char *buffer, size_t capacity, const sockaddr *addr, size_t addrlen) {
  if (addr->sa_family == AF_INET6) {
    return ipv6AddressToString(buffer, capacity, (const struct sockaddr_in6 *)addr, addrlen);
  }
  return ipv4AddressToString(buffer, capacity, (const struct sockaddr_in *)addr, addrlen);
}

std::string popenCall(const std::string &command) {
  std::array<char, 8192> buffer{};
  std::string result;
  FILE *pipe = popen(command.c_str(), "r");
  if (pipe == nullptr) {
    throw std::runtime_error("popen() failed!");
  }
  try {
    std::size_t bytesRead{};
    while ((bytesRead = std::fread(buffer.data(), sizeof(buffer.at(0)), sizeof(buffer), pipe)) != 0) {
      result += std::string(buffer.data(), bytesRead);
    }
  } catch (...) {
    pclose(pipe);
    throw;
  }
  if (!result.empty()) {
    if (result.back() == '\n') {
      result.pop_back();
    }
  }
  return result;
}

std::string getHostInfo() {
#ifndef _WIN32
  struct ifreq ifr;
  struct ifconf ifc;
  char buf[1024];
  int success = 0;

  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock == -1) { /* handle error*/
  };

  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) { /* handle error */
  }
  struct ifreq *it = ifc.ifc_req;
  const struct ifreq *const end = it + (ifc.ifc_len / sizeof(struct ifreq));

  std::string hostAddress;
  std::string hostName = popenCall("hostname");
  for (; it != end; ++it) {
    auto address = it->ifr_addr;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    getnameinfo(&address, sizeof(struct sockaddr), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST);
    hostAddress = std::string(hbuf);
    std::string interface_address = std::string(hbuf);
    strcpy(ifr.ifr_name, it->ifr_name);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
      if (!(ifr.ifr_flags & IFF_LOOPBACK)) {  // don't count loopback
        if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
          success = 1;
          break;
        }
      }
    } else { /* handle error */
    }
  }

  unsigned char mac_address[6];

  if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
  char *mac_address_string = new char[40];
  sprintf(mac_address_string, "%02X:%02X:%02X:%02X:%02X:%02X", mac_address[0], mac_address[1], mac_address[2],
          mac_address[3], mac_address[4], mac_address[5]);
  return std::string(mac_address_string) + "@" + hostAddress + "@" + hostName;
#else
        return "";
#endif
}

std::vector<std::string> split_string(const std::string &input_string, const std::regex &sep_regex) {
  std::sregex_token_iterator iter(input_string.begin(), input_string.end(), sep_regex, -1);
  std::sregex_token_iterator end;
  return {iter, end};
}

}  // namespace mdns_cpp
