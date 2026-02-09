
import argparse
import jinja2
import ProtoParser

def main():
    
    arg_parser = argparse.ArgumentParser(description= 'PacketGenerator')
    arg_parser.add_argument('--path', type=str, default='C:/Users\chanhee/Desktop/devCode/C++/Rookiss/Server/Common/protoc-3.21.12-win64/bin/Protocol.proto', help='proto path')
    arg_parser.add_argument('--output', type=str, default='TestPacketHandler', help='output file')
    arg_parser.add_argument('--lang', type=str, default='.h', help='language')
    arg_parser.add_argument('--server', type=str, default='', help='server type')
    args = arg_parser.parse_args()

    parser = ProtoParser.EnumParser(login_range_start=1000, game_range_start=2000, chat_range_start=3000)
    parser.parse_proto(args.path)
    
    # jinja2
    file_loader = jinja2.FileSystemLoader('Templates', encoding='utf-8')
    env = jinja2.Environment(loader=file_loader)
    
    generator_handler(env, parser, args.output, args.lang, args.server)
    return

def generator_handler(env, parser, output_name, lang, server):
    output_filename = output_name + lang
    template = env.get_template(server + output_filename)
    
    output = template.render(parser=parser, handler_name=output_name)
    
    with open(output_filename, 'w+', encoding='utf-8') as f:
        f.write(output)

if __name__ == '__main__':
    main()    