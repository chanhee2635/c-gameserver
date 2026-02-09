import re

class EnumParser(object):
    def __init__(self, login_range_start=1000, game_range_start=2000, chat_range_start=3000):
        self.login_client_packets = [] # 로그인 클라 패킷 목록
        self.login_server_packets = [] # 로그인 서버 패킷 목록
        self.game_client_packets = [] # 게임 클라 패킷 목록
        self.game_server_packets = [] # 게임 서버 패킷 목록
        self.chat_client_packets = [] # 채팅 클라 패킷 목록
        self.chat_server_packets = [] # 채팅 서버 패킷 목록
        self.total_client_packets = [] # 모든 클라 패킷 목록
        self.total_server_packets = [] # 모든 서버 패킷 목록
        self.login_range_start = login_range_start
        self.game_range_start = game_range_start
        self.chat_range_start = chat_range_start
        
        
    def parse_proto(self, path):
        with open(path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            
        pattern = re.compile(r'^\s*([A-Z_0-9]+)\s*=\s*([0-9]+)\s*;')
        
        for line in lines:
            match = pattern.search(line)
            if match :
                pkt_name = match.group(1).strip()
                pkt_id = int(match.group(2))

                if pkt_name.startswith('C_'):
                    if self.login_range_start <= pkt_id < self.game_range_start:
                        self.login_client_packets.append(Packet(pkt_name, pkt_id))
                        self.total_client_packets.append(Packet(pkt_name, pkt_id))
                    elif self.game_range_start <= pkt_id < self.game_range_start + 1000:
                        self.game_client_packets.append(Packet(pkt_name,pkt_id))
                        self.total_client_packets.append(Packet(pkt_name,pkt_id))
                    elif self.chat_range_start <= pkt_id < self.game_range_start + 2000:
                        self.chat_client_packets.append(Packet(pkt_name,pkt_id))
                        self.total_client_packets.append(Packet(pkt_name,pkt_id))
                elif pkt_name.startswith('S_'):
                    if self.login_range_start <= pkt_id < self.game_range_start:
                        self.login_server_packets.append(Packet(pkt_name, pkt_id))
                        self.total_server_packets.append(Packet(pkt_name, pkt_id))
                    elif self.game_range_start <= pkt_id < self.game_range_start + 1000:
                        self.game_server_packets.append(Packet(pkt_name,pkt_id))
                        self.total_server_packets.append(Packet(pkt_name,pkt_id))
                    elif self.chat_range_start <= pkt_id < self.game_range_start + 2000:
                        self.chat_server_packets.append(Packet(pkt_name,pkt_id))
                        self.total_server_packets.append(Packet(pkt_name,pkt_id))

class Packet:
    def __init__(self, name, id):
        self.name = name
        self.id = id
        self.camel_name = self.convert_to_camel_case(name)
    
    def convert_to_camel_case(self, name):
        if not name:
            return ""
        
        segments = name.split('_')
        prefix = segments[0]
        message_segments = segments[1:]
        camel_segments = [s[0].upper() + s[1:].lower() if s else '' for s in message_segments]
        camel_message_name = ''.join(camel_segments)
        return f"{prefix}_{camel_message_name}"