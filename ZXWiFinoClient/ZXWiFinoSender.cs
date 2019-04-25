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

        const int MaxPacketSize = 250 - 15;//Our buffer size less the IPD data

        public static event EventHandler<ProgressInfoEventArgs> ProgressChanged;

        public static async Task<bool> SendFile(string FileName, string TargetPath, string Server)
        {

            TcpClient c = null;
            NetworkStream ns = null;
            StreamReader sr = null;
            StreamWriter sw = null;

            try
            {
                int retries = 0;
                bool connected = false;

                while (!connected && retries++ < 6)
                {
                    try
                    {
                        c = new TcpClient();
                        await c.ConnectAsync(Server, 9090);
                        ns = c.GetStream();
                        ns.ReadTimeout = 10000;
                        ns.WriteTimeout = 10000;
                        sr = new StreamReader(ns);
                        sw = new StreamWriter(ns);

                        sr.ReadLine();
                        connected = true;
                    }
                    catch
                    {
                        try { sw.Dispose(); } catch { }
                        try { sr.Dispose(); } catch { }
                        try { ns.Dispose(); } catch { }
                        try { c.Dispose(); } catch { }
                    }
                }

                if (!connected)
                    return false;

                var line = TargetPath + Path.GetFileName(FileName) + "\r\n";
                sw.Write(line);
                sw.Flush();

                byte[] data = File.ReadAllBytes(FileName);

                int pos = 0;

                sr.ReadLine();

                while (pos < data.Length)
                {
                    int consume = Math.Min(data.Length - pos, MaxPacketSize);

                    byte[] escapedData = EscapeData(data, pos, consume, out consume);
                    
                    pos += consume;
                    await ns.WriteAsync(escapedData, 0, escapedData.Length);
                    await ns.FlushAsync();

                    if (ProgressChanged != null)
                        ProgressChanged(null, new ProgressInfoEventArgs { Sent = pos, TotalSize = data.Length });

                    sr.ReadLine();
                }

                byte[] endData = new byte[4] { 0xAA, 0x02, 0x0D, 0x0A };
                
                await ns.WriteAsync(endData, 0, endData.Length);
                await ns.FlushAsync();
                await Task.Delay(200);
                c.Close();

                return true;
            }
            catch { return false; }
            finally
            {
                try { sw.Dispose(); } catch { }
                try { sr.Dispose(); } catch { }
                try { ns.Dispose(); } catch { }
                try { c.Dispose(); } catch { }
            }
        }

        static byte[] EscapeData(byte[] Data, int Pos, int MaxLen, out int Consumed)
        {
            List<byte> data = new List<byte>();
            Consumed = 0;

            for (int buc = 0; buc < MaxLen; buc++)
            {
                if (Data[buc + Pos] == 0x0A)
                {
                    if (data.Count + 2 > MaxLen)
                        break;

                    data.Add(0xAA);
                    data.Add(0x00);
                    
                }
                else if (Data[buc + Pos] == 0xAA)
                {
                    if (data.Count + 2 > MaxLen)
                        break;

                    data.Add(0xAA);
                    data.Add(0x01);
                }
                else
                {
                    if (data.Count + 1 > MaxLen)
                        break;

                    data.Add(Data[buc + Pos]);
                }

                Consumed++;
            }

            data.Add(0x0D);
            data.Add(0x0A);

            return data.ToArray();
        }
        
    }

    public class ProgressInfoEventArgs : EventArgs
    {
        public int TotalSize { get; set; }
        public int Sent { get; set; }
    }
}
