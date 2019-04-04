﻿using System;
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
        string[] allowedFormats = new string[] { ".tap", ".tzx" };
        bool transferring = false;

        public Main()
        {
            InitializeComponent();
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
                if (files.Length == 1 && allowedFormats.Contains(Path.GetExtension(files[0]).ToLower()))
                {
                    e.Effect = DragDropEffects.Copy;
                }
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
                if (files.Length == 1 && allowedFormats.Contains(Path.GetExtension(files[0]).ToLower()))
                {
                    transferring = true;
                    string path = "";
                    if (rbSpecified.Checked)
                    {
                        path = txtFolder.Text.Replace("\\", "/");
                        if (!path.StartsWith("/"))
                            path = "/" + path;
                        if (!path.EndsWith("/"))
                            path = path + "/";
                    }
                    else
                    {
                        string fileName = Path.GetFileNameWithoutExtension(files[0]).ToUpper();
                        string letter = fileName.Substring(0, 1);
                        string letters = fileName.Substring(0, Math.Min(3, fileName.Length));
                        path = $"/{letter}/{letters}/";
                    }

                    pbTape.Image = Resources.cinta_llena;

                    Task.Run(async () =>
                    {
                        if (!await ZXWiFinoSender.SendFile(files[0], path, zxAddress.IPAddress.ToString()))
                        {
                            BeginInvoke((MethodInvoker)(() =>
                            {
                                MessageBox.Show("Error transferring file!");
                            }));
                        }
                        transferring = false;

                        BeginInvoke((MethodInvoker)(() =>
                        {
                            pBar.Value = 0;
                            pbTape.Image = Resources.cinta_vacía;
                        }));
                    });
                }
            }
        }
    }
}
