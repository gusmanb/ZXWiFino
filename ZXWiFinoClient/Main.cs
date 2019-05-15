using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using ZXWiFinoClient.Properties;

namespace ZXWiFinoClient
{
    public partial class Main : Form
    {
        string[] allowedFormats = new string[] { ".tap", ".tzx", ".cas" };
        bool transferring = false;

        public Main()
        {
            InitializeComponent();

            if (File.Exists(Path.Combine(Application.StartupPath, "lastaddress.dat")))
            {
                try
                {
                    zxAddress.IPAddress = IPAddress.Parse(File.ReadAllText(Path.Combine(Application.StartupPath, "lastaddress.dat")));
                }
                catch { }
            }

            ZXWiFinoSender.ProgressChanged += ZXWiFinoSender_ProgressChanged;
        }

        private void ZXWiFinoSender_ProgressChanged(object sender, ProgressInfoEventArgs e)
        {
            BeginInvoke((MethodInvoker)(() =>
            {
                pBar.Maximum = e.TotalSize;
                pBar.Value = e.Sent;
            }));
        }

        private void Main_DragEnter(object sender, DragEventArgs e)
        {
            if (transferring)
                return;

            if (zxAddress.IPAddress.ToString() == IPAddress.Any.ToString())
                return;

            if (rbSpecified.Checked && string.IsNullOrWhiteSpace(txtFolder.Text))
                return;

            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                var files = (string[])e.Data.GetData(DataFormats.FileDrop);

                foreach (var file in files)
                    if (!allowedFormats.Contains(Path.GetExtension(file).ToLower()))
                        return;

                e.Effect = DragDropEffects.Copy;
                
            }
        }

        private void Main_DragDrop(object sender, DragEventArgs e)
        {
            if (transferring)
                return;

            if (zxAddress.IPAddress.ToString() == IPAddress.Any.ToString())
                return;

            if (rbSpecified.Checked && string.IsNullOrWhiteSpace(txtFolder.Text))
                return;

            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                var files = (string[])e.Data.GetData(DataFormats.FileDrop);

                foreach (var file in files)
                    if (!allowedFormats.Contains(Path.GetExtension(file).ToLower()))
                        return;

                bool specified = rbSpecified.Checked;
                string tPath = txtFolder.Text;

                Task.Run(async () =>
                {
                    transferring = true;

                    foreach (var file in files)
                    {

                        string path = "";
                        if (specified)
                        {
                            path = tPath.Replace("\\", "/");
                            if (!path.StartsWith("/"))
                                path = "/" + path;
                            if (!path.EndsWith("/"))
                                path = path + "/";
                        }
                        else
                        {
                            string fileName = Path.GetFileNameWithoutExtension(file).ToUpper();
                            string letter = fileName.Substring(0, 1);
                            string letters = fileName.Substring(0, Math.Min(3, fileName.Length));
                            path = $"/{letter}/{letters}/";
                        }


                        BeginInvoke((MethodInvoker)(() =>
                        {
                            pbTape.Image = Resources.cinta_llena;
                            lblName.Text = Path.GetFileName(file);

                        }));

                        if (!await ZXWiFinoSender.SendFile(file, path, zxAddress.IPAddress.ToString()))
                        {
                            BeginInvoke((MethodInvoker)(() =>
                            {
                                MessageBox.Show("Error transferring file!");
                                transferring = false;
                                return;
                            }));
                        }
                        else
                        {
                            File.WriteAllText(Path.Combine(Application.StartupPath, "lastaddress.dat"), zxAddress.IPAddress.ToString());
                        }

                        BeginInvoke((MethodInvoker)(() =>
                        {
                            pBar.Value = 0;
                            lblName.Text = "";
                        }));

                        await Task.Delay(2000);
                    }

                    BeginInvoke((MethodInvoker)(() =>
                    {
                        pbTape.Image = Resources.cinta_vacía;

                    }));

                    transferring = false;
                });
            }
        }
    }
}
