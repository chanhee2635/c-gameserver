using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using UnityEngine;

namespace ServerCore
{
    public class ConnectContext
    {
        public Socket Socket { get; set; }
        public Func<Session> SessionFactory { get; set; }
    }

    public class Connector
    {
        public void Connect(IPEndPoint endPoint, Func<Session> sessionFactory, int count = 1)
        {
            for (int i = 0; i < count; i++)
            {
                Socket socket = new Socket(endPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

                ConnectContext context = new ConnectContext
                {
                    Socket = socket,
                    SessionFactory = sessionFactory,
                };

                SocketAsyncEventArgs args = new SocketAsyncEventArgs();
                args.Completed += OnConnectCompleted;
                args.RemoteEndPoint = endPoint;
                args.UserToken = context;

                RegisterConnect(args);
            }
        }

        void RegisterConnect(SocketAsyncEventArgs args)
        {
            ConnectContext context = args.UserToken as ConnectContext;
            if (context == null) return;

            bool pending = context.Socket.ConnectAsync(args);
            if (pending == false)
                OnConnectCompleted(null, args);
        }

        void OnConnectCompleted(object sender, SocketAsyncEventArgs args)
        {
            ConnectContext context = args.UserToken as ConnectContext;
            if (context == null) return;

            if (args.SocketError == SocketError.Success)
            {
                Session session = context.SessionFactory.Invoke();
                session.Start(args.ConnectSocket);
                session.OnConnected(args.RemoteEndPoint);
            }
            else
            {
                Debug.Log($"OnConnectCompleted Fail: {args.SocketError}");
            }

            args.Dispose();
        }
    }
}
