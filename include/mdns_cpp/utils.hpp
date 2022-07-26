#pragma once

#include <regex>
#include <string>
#include <vector>

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;

namespace mdns_cpp {

std::string getHostName();

std::string ipv4AddressToString(char *buffer, size_t capacity, const struct sockaddr_in *addr, size_t addrlen);

std::string ipv6AddressToString(char *buffer, size_t capacity, const struct sockaddr_in6 *addr, size_t addrlen);

std::string ipAddressToString(char *buffer, size_t capacity, const struct sockaddr *addr, size_t addrlen);

/**
 * Splits a string into a vector of strings based on a delimiter.
 *
 * @param input_string The string to split.
 * @param delimiter The delimiter to split the string on.
 *
 * @returns A vector of strings.
 */
std::vector<std::string> split_string(const std::string &input_string, const std::regex &sep_regex = std::regex{"@"});

#ifdef _WIN32

#else

/**
 * Executes a command through the shell.
 *
 * @param command The command to execute.
 *
 * @returns The output of the command.
 */
std::string popenCall(const std::string &command);

/**
 * Returns the host infos of the machine.
 *
 * @returns macAddress+@+hostAddress+@+hostName
 */
std::string getHostInfo();
#endif

}  // namespace mdns_cpp
