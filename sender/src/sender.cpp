#include <socket_data_sender/sender.h>
#include <string>
#include <socket_data/message_types.h>

bool Sender::cycle(){
    server->cycle();
    return true;
}

bool Sender::initialize(){
    server = new socket_connection::SocketConnectionHandler(logger);
    server->setSocketListener(this);
    server->openPortForRequests(config().get<int>("port",65111));
    return true;
}

bool Sender::deinitialize(){
    //TODO
    return false;
}

void Sender::receivedMessage(socket_connection::SocketConnector &from, char* buff, int bytesRead){
    char type = buff[0];
    logger.debug("Sender::receivedMessage") << "size: " <<bytesRead << "type: " << (int)type;
    switch ((MessageType)((int)type)) {
    case MessageType::CHANNEL_DATA:
        break;
    case MessageType::MESSAGE:
        break;
    case MessageType::CHANNEL_MAPPING:
        break;
    case MessageType::ERROR:
        break;
    case MessageType::GET_CHANNEL_DATA:
        //send the given channel to the client
        logger.debug("receivedMessage") << "GET_CHANNEL_DATA";
        sendChannelToClient(from,buff[1]);
        break;
    case MessageType::GET_CHANNEL_DATA_ALL:
        logger.debug("receivedMessage") << "GET_CHANNEL_DATA_ALL";
        sendChannelsToClient(from);
        break;
    case MessageType::REGISTER_CHANNEL:
        logger.error("register channels: ") << "clientID: " << from.getID();
        std::vector<char> &clientMapping = clientChannels[from.getID()];

        //split the string
        std::vector<std::string> channels = split(&buff[1],bytesRead-1,';');
        if(channels.size() == 0){
            logger.error("register channels: ") << "NO CHANNELS RECEIVED!";
            //no channels received!
            break;
        }

        std::string ans(1,(char)MessageType::CHANNEL_MAPPING);
        for(std::string &channel:channels){
            char channelID = addChannel(channel);
            clientMapping.push_back(channelID);
            logger.debug("REGISTER_CHANNEL") <<"NAME-"<< channel<<"---";
            ans += channel+";"+ channelID+";";
        }
        from.sendMessage(ans.c_str(),ans.length(),true);
        break;
    }
}

void Sender::sendChannelsToClient(socket_connection::SocketConnector &from){
    std::vector<char> &clientMapping = clientChannels[from.getID()];
    if(clientMapping.size() == 0){
        logger.error("Client tried to get channels but has no registered!") <<"clientID: " <<from.getID();
    }
    for(char channelID : clientMapping){
        sendChannelToClient(from,channelID);
    }

}

void Sender::sendChannelToClient(socket_connection::SocketConnector &from,char channelId){
    //TODO error checking
    //serialize channel and send it
    std::ostringstream osstream;
    //first byte is the typeID
    char c = (char)MessageType::CHANNEL_DATA;
    osstream.write(&c,1);
    //second byte is the channelID
    osstream.write(&channelId,1);
    //write the data into the stream
    readChannel<lms::Any>(channelMapping[channelId].name).serialize(osstream); //TODO use stored
    logger.debug("sendChannelToClient") << channelMapping[channelId].name << " bytesToSend: "<<osstream.str().length();
    from.sendMessage(osstream.str().c_str(),osstream.str().length(),true);
}

char Sender::addChannel(std::string name){
    //check if channel is already registered
    for(uint i = 0; i < channelMapping.size(); i++){
        if(channelMapping[i].name == name){
            return i;
        }
    }
    ChannelMapping cm;
    cm.name = name;
    cm.iD = channelMapping.size();
    channelMapping.push_back(cm);
    logger.debug("added Channel") << "name,id" << name <<","<< (int)cm.iD;
    //datamanager()->getReadAccess(this,name);
    readChannel<lms::Any>(name); //TODO store them
    return channelMapping.size() -1;
}

void Sender::disconnected(const socket_connection::SocketConnector &disconnected){
    if(clientChannels.find(disconnected.getID())!=clientChannels.end()){
        clientChannels.erase(disconnected.getID());
        logger.info("disconnected")<<"client disconnected: "<<disconnected.address;
    }else{
        logger.info("disconnected")<<"client is already disconnected: "<<disconnected.address;
    }
}

void Sender::connected(const socket_connection::SocketConnector &connected){
    if(clientChannels.find(connected.getID())==clientChannels.end()){
        clientChannels[connected.getID()];
        logger.info("connected")<<"client connected: "<<connected.address;
    }else{
        logger.error("connected")<<"client is already connected: "<<connected.address;
    }
}
