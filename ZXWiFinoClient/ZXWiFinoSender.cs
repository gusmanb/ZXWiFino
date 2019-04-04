using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace ZXWiFinoClient
{
    public static class ZXWiFinoSender
    {
        public static event EventHandler<ProgressInfoEventArgs> ProgressChanged;

        public static async Task<bool> SendFile(string FileName, string TargetPath, string Server)
        {
            try
            {
                TcpClient c = new TcpClient();
                await c.ConnectAsync(Server, 9090);
                NetworkStream ns = c.GetStream();
                ns.ReadTimeout = 10000;
                ns.WriteTimeout = 10000;
                StreamReader sr = new StreamReader(ns);
                StreamWriter sw = new StreamWriter(ns);

                sr.ReadLine();

                var line = TargetPath + Path.GetFileName(FileName) + "\r\n";
                sw.Write(line);
                sw.Flush();

                byte[] data = File.ReadAllBytes(FileName);

                int pos = 0;

                sr.ReadLine();

                while (pos < data.Length)
                {
                    int consume = Math.Min(data.Length - pos, 90);

                    line = Convert.ToBase64String(data, pos, consume) + "\r\n";
                    pos += consume;
                    sw.Write(line);
                    sw.Flush();

                    if (ProgressChanged != null)
                        ProgressChanged(null, new ProgressInfoEventArgs { Sent = pos, TotalSize = data.Length });

                    sr.ReadLine();
                }

                line = Convert.ToBase64String(new byte[] { 0 }) + "\r\n";
                sw.Write(line);
                sw.Flush();
                c.Close();
                return true;
            }
            catch { return false; }
        }
    }

    public class ProgressInfoEventArgs : EventArgs
    {
        public int TotalSize { get; set; }
        public int Sent { get; set; }
    }
}
