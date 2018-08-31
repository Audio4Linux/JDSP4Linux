namespace ViPERToolBox
{
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using System.Windows.Forms;

    public class frmEditPoint : Form
    {
        private Button button_Cancel;
        private Button button_OK;
        private MUI.MUIContent g_mcMUIContent;
        private Label label_Bandwidth;
        private Label label_Frequency;
        private Label label_Gain;
        private Label label_Tips_Bandwidth;
        private Label label_Tips_Frequency;
        private Label label_Tips_Gain;
        private double m_dCalibrationPointBW;
        private double m_dCalibrationPointGain;
        private int m_nCalibrationPointFreq;
        private TextBox textBox_Bandwidth;
        private TextBox textBox_Frequency;
        private TextBox textBox_Gain;

        public frmEditPoint(MUI.MUIContent mcMUIContent)
        {
            this.InitializeComponent();
            this.textBox_Frequency.Text = this.m_nCalibrationPointFreq.ToString();
            this.textBox_Bandwidth.Text = this.m_dCalibrationPointBW.ToString("F02");
            this.textBox_Gain.Text = this.m_dCalibrationPointGain.ToString("F02");
            this.g_mcMUIContent = mcMUIContent;
            this.RefreshMUI();
        }

        private void button_OK_Click(object sender, EventArgs e)
        {
            if (this.textBox_Frequency.Text.Length <= 0)
            {
                MessageBox.Show(this.g_mcMUIContent.Tip_Text_Invalid_Frequency, this.g_mcMUIContent.EditPointFormTitle, MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
            else if (this.textBox_Bandwidth.Text.Length <= 0)
            {
                MessageBox.Show(this.g_mcMUIContent.Tip_Text_Invalid_Bandwidth, this.g_mcMUIContent.EditPointFormTitle, MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
            else if (this.textBox_Gain.Text.Length <= 0)
            {
                MessageBox.Show(this.g_mcMUIContent.Tip_Text_Invalid_Gain, this.g_mcMUIContent.EditPointFormTitle, MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
            else if ((!int.TryParse(this.textBox_Frequency.Text, out this.m_nCalibrationPointFreq) || (this.m_nCalibrationPointFreq <= 0)) || (this.m_nCalibrationPointFreq >= 0x5dc0))
            {
                MessageBox.Show(this.g_mcMUIContent.Tip_Text_Invalid_Frequency, this.g_mcMUIContent.EditPointFormTitle, MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
            else if ((!double.TryParse(this.textBox_Bandwidth.Text, out this.m_dCalibrationPointBW) || (this.m_dCalibrationPointBW <= 0.0)) || (this.m_dCalibrationPointBW >= 100.0))
            {
                MessageBox.Show(this.g_mcMUIContent.Tip_Text_Invalid_Bandwidth, this.g_mcMUIContent.EditPointFormTitle, MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
            else if ((!double.TryParse(this.textBox_Gain.Text, out this.m_dCalibrationPointGain) || (this.m_dCalibrationPointGain < -24.0)) || (this.m_dCalibrationPointGain > 24.0))
            {
                MessageBox.Show(this.g_mcMUIContent.Tip_Text_Invalid_Gain, this.g_mcMUIContent.EditPointFormTitle, MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
            else
            {
                base.DialogResult = DialogResult.OK;
            }
        }

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
        }

        private void frmEditPoint_Load(object sender, EventArgs e)
        {
            this.textBox_Frequency.Text = this.m_nCalibrationPointFreq.ToString();
            this.textBox_Bandwidth.Text = this.m_dCalibrationPointBW.ToString("F02");
            this.textBox_Gain.Text = this.m_dCalibrationPointGain.ToString("F02");
        }

        private void InitializeComponent()
        {
            this.label_Frequency = new Label();
            this.textBox_Frequency = new TextBox();
            this.label_Tips_Frequency = new Label();
            this.label_Tips_Bandwidth = new Label();
            this.textBox_Bandwidth = new TextBox();
            this.label_Bandwidth = new Label();
            this.label_Tips_Gain = new Label();
            this.textBox_Gain = new TextBox();
            this.label_Gain = new Label();
            this.button_OK = new Button();
            this.button_Cancel = new Button();
            base.SuspendLayout();
            this.label_Frequency.AutoSize = true;
            this.label_Frequency.Location = new Point(0x2f, 0x2b);
            this.label_Frequency.Name = "label_Frequency";
            this.label_Frequency.Size = new Size(130, 0x18);
            this.label_Frequency.TabIndex = 0;
            this.label_Frequency.Text = "$FREQUENCY";
            this.textBox_Frequency.Location = new Point(0x33, 70);
            this.textBox_Frequency.Name = "textBox_Frequency";
            this.textBox_Frequency.Size = new Size(0xda, 0x23);
            this.textBox_Frequency.TabIndex = 1;
            this.label_Tips_Frequency.AutoSize = true;
            this.label_Tips_Frequency.Location = new Point(0x113, 0x4a);
            this.label_Tips_Frequency.Name = "label_Tips_Frequency";
            this.label_Tips_Frequency.Size = new Size(0x8e, 0x18);
            this.label_Tips_Frequency.TabIndex = 2;
            this.label_Tips_Frequency.Text = "(0 ~ 24000)";
            this.label_Tips_Bandwidth.AutoSize = true;
            this.label_Tips_Bandwidth.Location = new Point(0x113, 0xa7);
            this.label_Tips_Bandwidth.Name = "label_Tips_Bandwidth";
            this.label_Tips_Bandwidth.Size = new Size(0xa6, 0x18);
            this.label_Tips_Bandwidth.TabIndex = 5;
            this.label_Tips_Bandwidth.Text = "(0.0 ~ 100.0)";
            this.textBox_Bandwidth.Location = new Point(0x33, 0xa3);
            this.textBox_Bandwidth.Name = "textBox_Bandwidth";
            this.textBox_Bandwidth.Size = new Size(0xda, 0x23);
            this.textBox_Bandwidth.TabIndex = 4;
            this.label_Bandwidth.AutoSize = true;
            this.label_Bandwidth.Location = new Point(0x2f, 0x88);
            this.label_Bandwidth.Name = "label_Bandwidth";
            this.label_Bandwidth.Size = new Size(130, 0x18);
            this.label_Bandwidth.TabIndex = 3;
            this.label_Bandwidth.Text = "$BANDWIDTH";
            this.label_Tips_Gain.AutoSize = true;
            this.label_Tips_Gain.Location = new Point(0x113, 0x106);
            this.label_Tips_Gain.Name = "label_Tips_Gain";
            this.label_Tips_Gain.Size = new Size(190, 0x18);
            this.label_Tips_Gain.TabIndex = 8;
            this.label_Tips_Gain.Text = "[-24.0 ~ +24.0]";
            this.textBox_Gain.Location = new Point(0x33, 0x102);
            this.textBox_Gain.Name = "textBox_Gain";
            this.textBox_Gain.Size = new Size(0xda, 0x23);
            this.textBox_Gain.TabIndex = 7;
            this.label_Gain.AutoSize = true;
            this.label_Gain.Location = new Point(0x2f, 0xe7);
            this.label_Gain.Name = "label_Gain";
            this.label_Gain.Size = new Size(70, 0x18);
            this.label_Gain.TabIndex = 6;
            this.label_Gain.Text = "$GAIN";
            this.button_OK.Location = new Point(0x33, 0x14c);
            this.button_OK.Name = "button_OK";
            this.button_OK.Size = new Size(0x9d, 0x38);
            this.button_OK.TabIndex = 9;
            this.button_OK.Text = "$OK";
            this.button_OK.UseVisualStyleBackColor = true;
            this.button_OK.Click += new EventHandler(this.button_OK_Click);
            this.button_Cancel.DialogResult = DialogResult.Cancel;
            this.button_Cancel.Location = new Point(0x11c, 0x14c);
            this.button_Cancel.Name = "button_Cancel";
            this.button_Cancel.Size = new Size(0x9d, 0x38);
            this.button_Cancel.TabIndex = 10;
            this.button_Cancel.Text = "$CANCEL";
            this.button_Cancel.UseVisualStyleBackColor = true;
            base.AutoScaleDimensions = new SizeF(192f, 192f);
            base.AutoScaleMode = AutoScaleMode.Dpi;
            base.ClientSize = new Size(0x1e5, 0x1a0);
            base.Controls.Add(this.button_Cancel);
            base.Controls.Add(this.button_OK);
            base.Controls.Add(this.label_Tips_Gain);
            base.Controls.Add(this.textBox_Gain);
            base.Controls.Add(this.label_Gain);
            base.Controls.Add(this.label_Tips_Bandwidth);
            base.Controls.Add(this.textBox_Bandwidth);
            base.Controls.Add(this.label_Bandwidth);
            base.Controls.Add(this.label_Tips_Frequency);
            base.Controls.Add(this.textBox_Frequency);
            base.Controls.Add(this.label_Frequency);
            this.DoubleBuffered = true;
            base.FormBorderStyle = FormBorderStyle.FixedSingle;
            base.MaximizeBox = false;
            base.MinimizeBox = false;
            base.Name = "frmEditPoint";
            base.ShowInTaskbar = false;
            base.StartPosition = FormStartPosition.CenterScreen;
            this.Text = "$EDITPOINTFORMTITLE";
            base.Load += new EventHandler(this.frmEditPoint_Load);
            base.ResumeLayout(false);
            base.PerformLayout();
        }

        private void RefreshMUI()
        {
            this.Text = this.g_mcMUIContent.EditPointFormTitle;
            this.label_Frequency.Text = this.g_mcMUIContent.DDC_CPList_Freq;
            this.label_Bandwidth.Text = this.g_mcMUIContent.DDC_CPList_BW;
            this.label_Gain.Text = this.g_mcMUIContent.DDC_CPList_Gain;
            this.button_OK.Text = this.g_mcMUIContent.Tip_Text_OK;
            this.button_Cancel.Text = this.g_mcMUIContent.Tip_Text_Cancel;
        }

        public double CalibrationPointBandwidth
        {
            get
            {
                return this.m_dCalibrationPointBW;
            }
            set
            {
                this.m_dCalibrationPointBW = value;
            }
        }

        public int CalibrationPointFrequency
        {
            get
            {
                return this.m_nCalibrationPointFreq;
            }
            set
            {
                this.m_nCalibrationPointFreq = value;
            }
        }

        public double CalibrationPointGain
        {
            get
            {
                return this.m_dCalibrationPointGain;
            }
            set
            {
                this.m_dCalibrationPointGain = value;
            }
        }
    }
}

