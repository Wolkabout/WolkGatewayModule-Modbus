#ifndef PROTOCOLMANAGER_H
#define PROTOCOLMANAGER_H

#include <map>
#include <memory>
#include <string>

namespace wolkabout
{
class Protocol;
class DataProtocol;

class ProtocolManager
{
public:
    void registerDataProtocol(std::shared_ptr<DataProtocol> protocol);
    std::shared_ptr<DataProtocol> getDataProtocol(const std::string& protocolName);

private:
    std::map<std::string, std::shared_ptr<DataProtocol>> m_dataProtocols;
};
}    // namespace wolkabout

#endif    // PROTOCOLMANAGER_H
