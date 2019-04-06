namespace ZXWiFinoClient
{
    partial class Main
    {
        /// <summary>
        /// Variable del diseñador necesaria.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Limpiar los recursos que se estén usando.
        /// </summary>
        /// <param name="disposing">true si los recursos administrados se deben desechar; false en caso contrario.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Código generado por el Diseñador de Windows Forms

        /// <summary>
        /// Método necesario para admitir el Diseñador. No se puede modificar
        /// el contenido de este método con el editor de código.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Main));
            this.label1 = new System.Windows.Forms.Label();
            this.zxAddress = new IPAddressControlLib.IPAddressControl();
            this.rbAuto = new System.Windows.Forms.RadioButton();
            this.rbSpecified = new System.Windows.Forms.RadioButton();
            this.txtFolder = new System.Windows.Forms.TextBox();
            this.pbTape = new System.Windows.Forms.PictureBox();
            this.pBar = new System.Windows.Forms.ProgressBar();
            ((System.ComponentModel.ISupportInitialize)(this.pbTape)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 15);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(45, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "Address";
            // 
            // zxAddress
            // 
            this.zxAddress.AllowInternalTab = false;
            this.zxAddress.AutoHeight = true;
            this.zxAddress.BackColor = System.Drawing.SystemColors.Window;
            this.zxAddress.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.zxAddress.Cursor = System.Windows.Forms.Cursors.IBeam;
            this.zxAddress.Location = new System.Drawing.Point(63, 12);
            this.zxAddress.MinimumSize = new System.Drawing.Size(87, 20);
            this.zxAddress.Name = "zxAddress";
            this.zxAddress.ReadOnly = false;
            this.zxAddress.Size = new System.Drawing.Size(148, 20);
            this.zxAddress.TabIndex = 3;
            this.zxAddress.Text = "...";
            // 
            // rbAuto
            // 
            this.rbAuto.AutoSize = true;
            this.rbAuto.Checked = true;
            this.rbAuto.Location = new System.Drawing.Point(16, 38);
            this.rbAuto.Name = "rbAuto";
            this.rbAuto.Size = new System.Drawing.Size(147, 17);
            this.rbAuto.TabIndex = 4;
            this.rbAuto.TabStop = true;
            this.rbAuto.Text = "Automatically select folder";
            this.rbAuto.UseVisualStyleBackColor = true;
            // 
            // rbSpecified
            // 
            this.rbSpecified.AutoSize = true;
            this.rbSpecified.Location = new System.Drawing.Point(16, 61);
            this.rbSpecified.Name = "rbSpecified";
            this.rbSpecified.Size = new System.Drawing.Size(159, 17);
            this.rbSpecified.TabIndex = 5;
            this.rbSpecified.Text = "Place on the specified folder";
            this.rbSpecified.UseVisualStyleBackColor = true;
            // 
            // txtFolder
            // 
            this.txtFolder.Location = new System.Drawing.Point(16, 84);
            this.txtFolder.Name = "txtFolder";
            this.txtFolder.Size = new System.Drawing.Size(195, 20);
            this.txtFolder.TabIndex = 6;
            // 
            // pbTape
            // 
            this.pbTape.Image = global::ZXWiFinoClient.Properties.Resources.cinta_vacía;
            this.pbTape.Location = new System.Drawing.Point(15, 121);
            this.pbTape.Name = "pbTape";
            this.pbTape.Size = new System.Drawing.Size(196, 141);
            this.pbTape.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.pbTape.TabIndex = 0;
            this.pbTape.TabStop = false;
            this.pbTape.DragEnter += new System.Windows.Forms.DragEventHandler(this.Main_DragEnter);
            // 
            // pBar
            // 
            this.pBar.Location = new System.Drawing.Point(16, 268);
            this.pBar.Name = "pBar";
            this.pBar.Size = new System.Drawing.Size(195, 23);
            this.pBar.TabIndex = 7;
            // 
            // Main
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(227, 300);
            this.Controls.Add(this.pBar);
            this.Controls.Add(this.txtFolder);
            this.Controls.Add(this.rbSpecified);
            this.Controls.Add(this.rbAuto);
            this.Controls.Add(this.zxAddress);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.pbTape);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "Main";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "ZXWiFino Client";
            this.DragDrop += new System.Windows.Forms.DragEventHandler(this.Main_DragDrop);
            this.DragEnter += new System.Windows.Forms.DragEventHandler(this.Main_DragEnter);
            ((System.ComponentModel.ISupportInitialize)(this.pbTape)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox pbTape;
        private System.Windows.Forms.Label label1;
        private IPAddressControlLib.IPAddressControl zxAddress;
        private System.Windows.Forms.RadioButton rbAuto;
        private System.Windows.Forms.RadioButton rbSpecified;
        private System.Windows.Forms.TextBox txtFolder;
        private System.Windows.Forms.ProgressBar pBar;
    }
}

