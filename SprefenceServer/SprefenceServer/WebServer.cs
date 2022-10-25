using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Controls;

namespace SprefenceServer
{
    internal class WebServer
    {
        HttpListener m_listener;
        TextBlock m_logBox;
        SynchronizationContext m_mainThread;
        CancellationTokenSource m_source = new();
        string[] m_contents;
        byte[] m_buffer = new byte[1024 * 1024];
        DateTime updated;

        internal WebServer(TextBlock logbox, SynchronizationContext mainThread)
        {
            m_mainThread = mainThread;
            m_logBox = logbox;
            m_listener = new HttpListener();
            m_listener.Prefixes.Clear();
            m_listener.Prefixes.Add(@"http://+:8080/");
            m_contents = Directory.GetFiles("contents").Select(p => Path.GetFileName(p)).ToArray();
        }

        internal void Open(SerialPort port)
        {
            m_listener.Start();
            port.DataReceived += (s, e) =>
            {
                updated = DateTime.Now;/*
                if (e.EventType == SerialData.Chars)
                {
                    var sd = (SerialPort)s;
                    sd.Write("ack");
                }*/
            };
            Task.Run(() => Run(port, m_source.Token));
        }

        internal void Close()
        {
            m_listener?.Stop();
            m_source.Cancel();
        }

        private bool isEnd(int size)
        {
            var endWord = "EndOfFile\0".ToCharArray();
            for (var i = 0; i < 10; i++)
            {
                if (m_buffer[size - 10 + i] != endWord[i])
                {
                    return false;
                }
            }
            return true;
        }

        private async Task Run(SerialPort port, CancellationToken token)
        {
            while (true)
            {
                if (token.IsCancellationRequested)
                {
                    m_listener.Close();
                    m_mainThread.Send((_) =>
                    {
                        m_logBox.Text += "\nServer closed";
                    }, null);
                    return;
                }

                var context = await m_listener.GetContextAsync();
                var request = context.Request;
                var response = context.Response;
                var requestPath = Path.GetFileName(request?.Url?.LocalPath);
                if (request != null && requestPath != null)
                {
                    if (m_contents.Contains(requestPath))
                    {
                        var fileContent = File.ReadAllBytes(Path.Combine("contents", requestPath));
                        response.OutputStream.Write(fileContent);
                        m_mainThread.Send((_) =>
                        {
                            m_logBox.Text += "\nGet : " + requestPath;
                        }, null);
                    }
                    else if (requestPath == "camera.jpg")
                    {
                        port.Write("camera");
                        updated = DateTime.Now;
                        await Task.Delay(100);
                        var size = 0;
                        while (true)
                        {
                            while ((DateTime.Now - updated).TotalMilliseconds < 1500)
                            {
                                await Task.Delay(100);
                            }
                            size += port.Read(m_buffer, size, port.BytesToRead);
                            if (isEnd(size))
                            {
                                break;
                            }
                        }
                        List<byte> res = new List<byte>();
                        for (int i = 0; i < (size - 10) / 8.0; i++)
                        {
                            var len = size - i * 8;
                            if (len > 8)
                            {
                                len = 8;
                            }
                            var a_byte = m_buffer[8 * i + len - 1] - 32;
                            if (a_byte >= 127)
                            {
                                a_byte -= 33;
                            }

                            for (int j = 0; j < len - 1; j++)
                            {
                                var d_byte = m_buffer[8 * i + j] - 32;
                                if (d_byte >= 127)
                                {
                                    d_byte -= 33;
                                }
                                res.Add((byte)(d_byte | (0b_1000_0000 & (a_byte << (j + 1)))));
                            }
                        }
                        response.OutputStream.Write(res.ToArray(), 0, res.Count);
                        m_mainThread.Send((_) =>
                        {
                            m_logBox.Text += "\nGet : " + requestPath + " " + res.Count + " bytes";
                        }, null);
                    }
                    else if (request.HttpMethod == HttpMethod.Post.Method)
                    {
                        var body = new StreamReader(request.InputStream, request.ContentEncoding).ReadToEnd();
                        var points = JsonSerializer.Deserialize<Points>(body);
                        var top = points?.points?.OrderBy(p => p.y);
                        var lt = top?.First().x < top?.Skip(1).First().x ? top.First() : top?.Skip(1).First();
                        var rt = top?.First().x < top?.Skip(1).First().x ? top?.Skip(1).First() : top?.First();
                        var lb = top?.Skip(2).First().x < top?.Skip(3).First().x ? top?.Skip(2).First() : top?.Skip(3).First();
                        var rb = top?.Skip(2).First().x < top?.Skip(3).First().x ? top?.Skip(3).First() : top?.Skip(2).First();
                        var dlt = (lt?.y - lb?.y) / (lt?.x - lb?.x);
                        var drt = (rt?.y - rb?.y) / (rt?.x - rb?.x);
                        var ltp = lt.x + (1 - lt.y) / dlt;
                        var lbp = lb.x + lb.y / dlt;
                        var rtp = rt.x + (1 - rt.y) / drt;
                        var rbp = rt.x + rt.y / drt;
                        port.Write("set " + ltp?.ToString("0.00") + " " + lbp?.ToString("0.00") + " " + rtp?.ToString("0.00") + " " + rbp?.ToString("0.00"));
                    }
                    else
                    {
                        response.StatusCode = 404;
                    }
                }
                else
                {
                    response.StatusCode = 404;
                }
                response.Close();
            }
        }
    }
}
