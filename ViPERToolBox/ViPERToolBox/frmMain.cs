namespace ViPERToolBox
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Drawing2D;
    using System.Drawing.Text;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;

    public class frmMain : Form
    {
        private Button button_DDCExportVDC;
        private Button button_OpenDDCProject;
        private Button button_SaveDDCProject;
        private ColumnHeader columnHeader_DDCCPList_BW;
        private ColumnHeader columnHeader_DDCCPList_Freq;
        private ColumnHeader columnHeader_DDCCPList_Gain;
        private IContainer components;
        private ContextMenuStrip contextMenuStrip_DDCList;
        private DDCContext g_dcDDCContext = new DDCContext();
        private MUI.MUIContent g_mcMUIContent;
        private Mutex g_mtxDDCListMutex = new Mutex();
        private DDCPointsSorter g_psDDCSorter = new DDCPointsSorter();
        private GroupBox groupBox_DDCPoints;
        private ListView listView_DDCPoints;
        private MenuStrip menuStrip_Main;
        private OpenFileDialog openFileDialog_DDCProject;
        private Panel panel_DDCView;
        private SaveFileDialog saveFileDialog_DDCProject;
        private SaveFileDialog saveFileDialog_VDCData;
        private TabControl tabControl_Main;
        private TabPage tabPage_DDCMode;
        private ToolStripMenuItem toolStripMenuItem_DDCCPList_Add;
        private ToolStripMenuItem toolStripMenuItem_DDCCPList_Chg;
        private ToolStripMenuItem toolStripMenuItem_DDCCPList_Clr;
        private ToolStripMenuItem toolStripMenuItem_DDCCPList_RM;
        private ToolStripMenuItem toolStripMenuItem_Lan;
        private ToolStripMenuItem toolStripMenuItem_LAN_eng;
        private ToolStripMenuItem toolStripMenuItem_LAN_zh_CN;
        private ToolStripMenuItem toolStripMenuItem_LAN_zh_TW;

        public frmMain()
        {
            this.InitializeComponent();
            this.listView_DDCPoints.ListViewItemSorter = this.g_psDDCSorter;
            int widthOfList = (listView_DDCPoints.ClientRectangle.Width - 3) / 3;
            this.listView_DDCPoints.Columns[0].Width = widthOfList;
            this.listView_DDCPoints.Columns[1].Width = widthOfList;
            this.listView_DDCPoints.Columns[2].Width = widthOfList;
            MUI.LoadResources(CultureInfo.CurrentCulture.Name, out this.g_mcMUIContent);
            this.RefreshMUI();
        }

        private void button_DDCExportVDC_Click(object sender, EventArgs e)
        {
            double[] numArray = this.g_dcDDCContext.ExportCoeffs(44100.0);
            double[] numArray2 = this.g_dcDDCContext.ExportCoeffs(48000.0);
            if ((numArray == null) || (numArray2 == null))
            {
                MessageBox.Show(this.g_mcMUIContent.Tip_Text_Failed_To_Export_VDC, this.g_mcMUIContent.MainFormTitle, MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
            else if (this.saveFileDialog_VDCData.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    TextWriter writer = new StreamWriter(this.saveFileDialog_VDCData.FileName, false, Encoding.ASCII);
                    writer.Write("SR_44100:");
                    for (int i = 0; i < numArray.Length; i++)
                    {
                        if (i == (numArray.Length - 1))
                        {
                            writer.Write(numArray[i]);
                        }
                        else
                        {
                            writer.Write(numArray[i]);
                            writer.Write(",");
                        }
                    }
                    writer.Write("\n");
                    writer.Write("SR_48000:");
                    for (int j = 0; j < numArray2.Length; j++)
                    {
                        if (j == (numArray2.Length - 1))
                        {
                            writer.Write(numArray2[j]);
                        }
                        else
                        {
                            writer.Write(numArray2[j]);
                            writer.Write(",");
                        }
                    }
                    writer.Write("\n");
                    writer.Flush();
                    writer.Close();
                    writer = null;
                }
                catch (Exception exception)
                {
                    Trace.WriteLine(exception.Message);
                }
            }
        }

        private void button_OpenDDCProject_Click(object sender, EventArgs e)
        {
            if (this.openFileDialog_DDCProject.ShowDialog() == DialogResult.OK)
            {
                this.g_mtxDDCListMutex.WaitOne();
                this.listView_DDCPoints.Items.Clear();
                this.g_dcDDCContext.ClearFilters();
                try
                {
                    string str;
                    TextReader reader = new StreamReader(this.openFileDialog_DDCProject.FileName, Encoding.Unicode);
                Label_004E:
                    str = reader.ReadLine().Trim();
                    if (str != null)
                    {
                        if ((str.Length > 0) && !str.StartsWith("#"))
                        {
                            string[] strArray = str.Split(new char[] { ',' });
                            if ((strArray != null) && (strArray.Length == 3))
                            {
                                int result = 0;
                                double num2 = 0.0;
                                double num3 = 0.0;
                                if ((int.TryParse(strArray[0], out result) && double.TryParse(strArray[1], out num2)) && double.TryParse(strArray[2], out num3))
                                {
                                    bool flag = false;
                                    for (int i = 0; i < this.listView_DDCPoints.Items.Count; i++)
                                    {
                                        if (this.listView_DDCPoints.Items[i].Text == result.ToString())
                                        {
                                            flag = true;
                                            break;
                                        }
                                    }
                                    if (!flag)
                                    {
                                        ListViewItem item = new ListViewItem {
                                            Text = result.ToString()
                                        };
                                        item.SubItems.Add(num2.ToString("F02"));
                                        item.SubItems.Add(num3.ToString("F02"));
                                        this.listView_DDCPoints.Items.Add(item);
                                        this.g_dcDDCContext.AddFilter(result, num3, num2, 44100.0);
                                    }
                                }
                            }
                        }
                        goto Label_004E;
                    }
                    reader.Close();
                    reader = null;
                    this.listView_DDCPoints.Sort();
                }
                catch (Exception exception)
                {
                    Trace.WriteLine(exception.Message);
                    this.g_mtxDDCListMutex.ReleaseMutex();
                    this.panel_DDCView.Invalidate();
                    return;
                }
                this.g_mtxDDCListMutex.ReleaseMutex();
                this.panel_DDCView.Invalidate();
            }
        }

        private void button_SaveDDCProject_Click(object sender, EventArgs e)
        {
            if (this.saveFileDialog_DDCProject.ShowDialog() == DialogResult.OK)
            {
                this.g_mtxDDCListMutex.WaitOne();
                try
                {
                    TextWriter writer = new StreamWriter(this.saveFileDialog_DDCProject.FileName, false, Encoding.Unicode);
                    writer.WriteLine("# ViPER-DDC Project File, v1.0.0.0");
                    writer.WriteLine();
                    for (int i = 0; i < this.listView_DDCPoints.Items.Count; i++)
                    {
                        writer.WriteLine("# Calibration Point " + ((i + 1)).ToString());
                        writer.WriteLine(this.listView_DDCPoints.Items[i].Text + "," + this.listView_DDCPoints.Items[i].SubItems[1].Text + "," + this.listView_DDCPoints.Items[i].SubItems[2].Text);
                    }
                    writer.WriteLine();
                    writer.WriteLine("# File End");
                    writer.Flush();
                    writer.Close();
                    writer = null;
                }
                catch (Exception exception)
                {
                    Trace.WriteLine(exception.Message);
                    this.g_mtxDDCListMutex.ReleaseMutex();
                    return;
                }
                this.g_mtxDDCListMutex.ReleaseMutex();
            }
        }

        private void contextMenuStrip_DDCList_Opening(object sender, CancelEventArgs e)
        {
            ContextMenuStrip strip = sender as ContextMenuStrip;
            if (strip.Tag == null)
            {
                this.toolStripMenuItem_DDCCPList_Add.Enabled = false;
                this.toolStripMenuItem_DDCCPList_Chg.Enabled = false;
                this.toolStripMenuItem_DDCCPList_RM.Enabled = false;
                this.toolStripMenuItem_DDCCPList_Clr.Enabled = false;
                e.Cancel = true;
                return;
            }
            if (strip.Tag.GetType() != typeof(string))
            {
                this.toolStripMenuItem_DDCCPList_Add.Enabled = false;
                this.toolStripMenuItem_DDCCPList_Chg.Enabled = false;
                this.toolStripMenuItem_DDCCPList_RM.Enabled = false;
                this.toolStripMenuItem_DDCCPList_Clr.Enabled = false;
                e.Cancel = true;
                return;
            }
            this.g_mtxDDCListMutex.WaitOne();
            string tag = strip.Tag as string;
            string str2 = tag.ToLower().Trim();
            if (str2 != null)
            {
                if (!(str2 == "cplist"))
                {
                    if (str2 == "ddcview")
                    {
                        if (this.listView_DDCPoints.Items.Count <= 0)
                        {
                            this.toolStripMenuItem_DDCCPList_Add.Enabled = true;
                            this.toolStripMenuItem_DDCCPList_Chg.Enabled = false;
                            this.toolStripMenuItem_DDCCPList_RM.Enabled = false;
                            this.toolStripMenuItem_DDCCPList_Clr.Enabled = false;
                            e.Cancel = false;
                        }
                        else
                        {
                            this.toolStripMenuItem_DDCCPList_Add.Enabled = true;
                            this.toolStripMenuItem_DDCCPList_Chg.Enabled = false;
                            this.toolStripMenuItem_DDCCPList_RM.Enabled = false;
                            this.toolStripMenuItem_DDCCPList_Clr.Enabled = true;
                            e.Cancel = false;
                        }
                        goto Label_0310;
                    }
                }
                else
                {
                    if (this.listView_DDCPoints.Items.Count <= 0)
                    {
                        this.toolStripMenuItem_DDCCPList_Add.Enabled = true;
                        this.toolStripMenuItem_DDCCPList_Chg.Enabled = false;
                        this.toolStripMenuItem_DDCCPList_RM.Enabled = false;
                        this.toolStripMenuItem_DDCCPList_Clr.Enabled = false;
                        e.Cancel = false;
                    }
                    else if (this.listView_DDCPoints.SelectedItems == null)
                    {
                        this.toolStripMenuItem_DDCCPList_Add.Enabled = true;
                        this.toolStripMenuItem_DDCCPList_Chg.Enabled = false;
                        this.toolStripMenuItem_DDCCPList_RM.Enabled = false;
                        this.toolStripMenuItem_DDCCPList_Clr.Enabled = true;
                        e.Cancel = false;
                    }
                    else if (this.listView_DDCPoints.SelectedItems.Count <= 0)
                    {
                        this.toolStripMenuItem_DDCCPList_Add.Enabled = true;
                        this.toolStripMenuItem_DDCCPList_Chg.Enabled = false;
                        this.toolStripMenuItem_DDCCPList_RM.Enabled = false;
                        this.toolStripMenuItem_DDCCPList_Clr.Enabled = true;
                        e.Cancel = false;
                    }
                    else if (this.listView_DDCPoints.SelectedItems.Count == 1)
                    {
                        this.toolStripMenuItem_DDCCPList_Add.Enabled = true;
                        this.toolStripMenuItem_DDCCPList_Chg.Enabled = true;
                        this.toolStripMenuItem_DDCCPList_RM.Enabled = true;
                        this.toolStripMenuItem_DDCCPList_Clr.Enabled = true;
                        e.Cancel = false;
                    }
                    else
                    {
                        this.toolStripMenuItem_DDCCPList_Add.Enabled = true;
                        this.toolStripMenuItem_DDCCPList_Chg.Enabled = false;
                        this.toolStripMenuItem_DDCCPList_RM.Enabled = true;
                        this.toolStripMenuItem_DDCCPList_Clr.Enabled = true;
                        e.Cancel = false;
                    }
                    goto Label_0310;
                }
            }
            this.toolStripMenuItem_DDCCPList_Add.Enabled = false;
            this.toolStripMenuItem_DDCCPList_Chg.Enabled = false;
            this.toolStripMenuItem_DDCCPList_RM.Enabled = false;
            this.toolStripMenuItem_DDCCPList_Clr.Enabled = false;
            e.Cancel = true;
        Label_0310:
            this.g_mtxDDCListMutex.ReleaseMutex();
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing && (this.components != null))
            {
                this.components.Dispose();
            }
            base.Dispose(disposing);
        }
        
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.menuStrip_Main = new System.Windows.Forms.MenuStrip();
            this.toolStripMenuItem_Lan = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem_LAN_eng = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem_LAN_zh_CN = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem_LAN_zh_TW = new System.Windows.Forms.ToolStripMenuItem();
            this.tabControl_Main = new System.Windows.Forms.TabControl();
            this.tabPage_DDCMode = new System.Windows.Forms.TabPage();
            this.button_DDCExportVDC = new System.Windows.Forms.Button();
            this.button_SaveDDCProject = new System.Windows.Forms.Button();
            this.button_OpenDDCProject = new System.Windows.Forms.Button();
            this.panel_DDCView = new System.Windows.Forms.Panel();
            this.groupBox_DDCPoints = new System.Windows.Forms.GroupBox();
            this.listView_DDCPoints = new System.Windows.Forms.ListView();
            this.columnHeader_DDCCPList_Freq = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader_DDCCPList_BW = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader_DDCCPList_Gain = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.contextMenuStrip_DDCList = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.toolStripMenuItem_DDCCPList_Add = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem_DDCCPList_Chg = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem_DDCCPList_RM = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem_DDCCPList_Clr = new System.Windows.Forms.ToolStripMenuItem();
            this.openFileDialog_DDCProject = new System.Windows.Forms.OpenFileDialog();
            this.saveFileDialog_DDCProject = new System.Windows.Forms.SaveFileDialog();
            this.saveFileDialog_VDCData = new System.Windows.Forms.SaveFileDialog();
            this.menuStrip_Main.SuspendLayout();
            this.tabControl_Main.SuspendLayout();
            this.tabPage_DDCMode.SuspendLayout();
            this.groupBox_DDCPoints.SuspendLayout();
            this.contextMenuStrip_DDCList.SuspendLayout();
            this.SuspendLayout();
            // 
            // menuStrip_Main
            // 
            this.menuStrip_Main.ImageScalingSize = new System.Drawing.Size(32, 32);
            this.menuStrip_Main.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripMenuItem_Lan});
            this.menuStrip_Main.Location = new System.Drawing.Point(0, 0);
            this.menuStrip_Main.Name = "menuStrip_Main";
            this.menuStrip_Main.Padding = new System.Windows.Forms.Padding(3, 1, 0, 1);
            this.menuStrip_Main.Size = new System.Drawing.Size(715, 24);
            this.menuStrip_Main.TabIndex = 0;
            // 
            // toolStripMenuItem_Lan
            // 
            this.toolStripMenuItem_Lan.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripMenuItem_LAN_eng,
            this.toolStripMenuItem_LAN_zh_CN,
            this.toolStripMenuItem_LAN_zh_TW});
            this.toolStripMenuItem_Lan.Name = "toolStripMenuItem_Lan";
            this.toolStripMenuItem_Lan.Size = new System.Drawing.Size(92, 22);
            this.toolStripMenuItem_Lan.Text = "$LANGUAGE";
            // 
            // toolStripMenuItem_LAN_eng
            // 
            this.toolStripMenuItem_LAN_eng.Name = "toolStripMenuItem_LAN_eng";
            this.toolStripMenuItem_LAN_eng.Size = new System.Drawing.Size(189, 22);
            this.toolStripMenuItem_LAN_eng.Text = "$LANGUAGE_eng";
            this.toolStripMenuItem_LAN_eng.Click += new System.EventHandler(this.toolStripMenuItem_LAN_eng_Click);
            // 
            // toolStripMenuItem_LAN_zh_CN
            // 
            this.toolStripMenuItem_LAN_zh_CN.Name = "toolStripMenuItem_LAN_zh_CN";
            this.toolStripMenuItem_LAN_zh_CN.Size = new System.Drawing.Size(189, 22);
            this.toolStripMenuItem_LAN_zh_CN.Text = "$LANGUAGE_zh_CN";
            this.toolStripMenuItem_LAN_zh_CN.Click += new System.EventHandler(this.toolStripMenuItem_LAN_zh_CN_Click);
            // 
            // toolStripMenuItem_LAN_zh_TW
            // 
            this.toolStripMenuItem_LAN_zh_TW.Name = "toolStripMenuItem_LAN_zh_TW";
            this.toolStripMenuItem_LAN_zh_TW.Size = new System.Drawing.Size(189, 22);
            this.toolStripMenuItem_LAN_zh_TW.Text = "$LANGUAGE_zh_TW";
            this.toolStripMenuItem_LAN_zh_TW.Click += new System.EventHandler(this.toolStripMenuItem_LAN_zh_TW_Click);
            // 
            // tabControl_Main
            // 
            this.tabControl_Main.Controls.Add(this.tabPage_DDCMode);
            this.tabControl_Main.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabControl_Main.Location = new System.Drawing.Point(0, 24);
            this.tabControl_Main.Margin = new System.Windows.Forms.Padding(2);
            this.tabControl_Main.Name = "tabControl_Main";
            this.tabControl_Main.SelectedIndex = 0;
            this.tabControl_Main.Size = new System.Drawing.Size(715, 390);
            this.tabControl_Main.TabIndex = 1;
            // 
            // tabPage_DDCMode
            // 
            this.tabPage_DDCMode.Controls.Add(this.button_DDCExportVDC);
            this.tabPage_DDCMode.Controls.Add(this.button_SaveDDCProject);
            this.tabPage_DDCMode.Controls.Add(this.button_OpenDDCProject);
            this.tabPage_DDCMode.Controls.Add(this.panel_DDCView);
            this.tabPage_DDCMode.Controls.Add(this.groupBox_DDCPoints);
            this.tabPage_DDCMode.Location = new System.Drawing.Point(4, 22);
            this.tabPage_DDCMode.Margin = new System.Windows.Forms.Padding(2);
            this.tabPage_DDCMode.Name = "tabPage_DDCMode";
            this.tabPage_DDCMode.Padding = new System.Windows.Forms.Padding(2);
            this.tabPage_DDCMode.Size = new System.Drawing.Size(707, 364);
            this.tabPage_DDCMode.TabIndex = 0;
            this.tabPage_DDCMode.Text = "$DDC_MODE";
            this.tabPage_DDCMode.UseVisualStyleBackColor = true;
            // 
            // button_DDCExportVDC
            // 
            this.button_DDCExportVDC.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button_DDCExportVDC.Location = new System.Drawing.Point(500, 320);
            this.button_DDCExportVDC.Margin = new System.Windows.Forms.Padding(2);
            this.button_DDCExportVDC.Name = "button_DDCExportVDC";
            this.button_DDCExportVDC.Size = new System.Drawing.Size(128, 40);
            this.button_DDCExportVDC.TabIndex = 4;
            this.button_DDCExportVDC.Text = "$DDC_EXPORT_VDC";
            this.button_DDCExportVDC.UseVisualStyleBackColor = true;
            this.button_DDCExportVDC.Click += new System.EventHandler(this.button_DDCExportVDC_Click);
            // 
            // button_SaveDDCProject
            // 
            this.button_SaveDDCProject.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button_SaveDDCProject.Location = new System.Drawing.Point(368, 320);
            this.button_SaveDDCProject.Margin = new System.Windows.Forms.Padding(2);
            this.button_SaveDDCProject.Name = "button_SaveDDCProject";
            this.button_SaveDDCProject.Size = new System.Drawing.Size(128, 40);
            this.button_SaveDDCProject.TabIndex = 3;
            this.button_SaveDDCProject.Text = "$DDC_SAVE_PROJECT";
            this.button_SaveDDCProject.UseVisualStyleBackColor = true;
            this.button_SaveDDCProject.Click += new System.EventHandler(this.button_SaveDDCProject_Click);
            // 
            // button_OpenDDCProject
            // 
            this.button_OpenDDCProject.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button_OpenDDCProject.Location = new System.Drawing.Point(236, 320);
            this.button_OpenDDCProject.Margin = new System.Windows.Forms.Padding(2);
            this.button_OpenDDCProject.Name = "button_OpenDDCProject";
            this.button_OpenDDCProject.Size = new System.Drawing.Size(128, 40);
            this.button_OpenDDCProject.TabIndex = 2;
            this.button_OpenDDCProject.Text = "$DDC_OPEN_PROJECT";
            this.button_OpenDDCProject.UseVisualStyleBackColor = true;
            this.button_OpenDDCProject.Click += new System.EventHandler(this.button_OpenDDCProject_Click);
            // 
            // panel_DDCView
            // 
            this.panel_DDCView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panel_DDCView.Location = new System.Drawing.Point(234, 2);
            this.panel_DDCView.Margin = new System.Windows.Forms.Padding(2);
            this.panel_DDCView.Name = "panel_DDCView";
            this.panel_DDCView.Size = new System.Drawing.Size(477, 314);
            this.panel_DDCView.TabIndex = 1;
            this.panel_DDCView.Paint += new System.Windows.Forms.PaintEventHandler(this.panel_DDCView_Paint);
            this.panel_DDCView.MouseUp += new System.Windows.Forms.MouseEventHandler(this.panel_DDCView_MouseUp);
            // 
            // groupBox_DDCPoints
            // 
            this.groupBox_DDCPoints.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.groupBox_DDCPoints.Controls.Add(this.listView_DDCPoints);
            this.groupBox_DDCPoints.Location = new System.Drawing.Point(2, 2);
            this.groupBox_DDCPoints.Margin = new System.Windows.Forms.Padding(2);
            this.groupBox_DDCPoints.Name = "groupBox_DDCPoints";
            this.groupBox_DDCPoints.Padding = new System.Windows.Forms.Padding(2);
            this.groupBox_DDCPoints.Size = new System.Drawing.Size(230, 360);
            this.groupBox_DDCPoints.TabIndex = 0;
            this.groupBox_DDCPoints.TabStop = false;
            this.groupBox_DDCPoints.Text = "$DDC_POINTS";
            // 
            // listView_DDCPoints
            // 
            this.listView_DDCPoints.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader_DDCCPList_Freq,
            this.columnHeader_DDCCPList_BW,
            this.columnHeader_DDCCPList_Gain});
            this.listView_DDCPoints.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listView_DDCPoints.FullRowSelect = true;
            this.listView_DDCPoints.GridLines = true;
            this.listView_DDCPoints.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.listView_DDCPoints.HideSelection = false;
            this.listView_DDCPoints.Location = new System.Drawing.Point(2, 17);
            this.listView_DDCPoints.Margin = new System.Windows.Forms.Padding(2);
            this.listView_DDCPoints.Name = "listView_DDCPoints";
            this.listView_DDCPoints.Size = new System.Drawing.Size(226, 341);
            this.listView_DDCPoints.TabIndex = 1;
            this.listView_DDCPoints.UseCompatibleStateImageBehavior = false;
            this.listView_DDCPoints.View = System.Windows.Forms.View.Details;
            this.listView_DDCPoints.MouseUp += new System.Windows.Forms.MouseEventHandler(this.listView_DDCPoints_MouseUp);
            // 
            // columnHeader_DDCCPList_Freq
            // 
            this.columnHeader_DDCCPList_Freq.Text = "$DDC_CPLIST_FREQ";
            this.columnHeader_DDCCPList_Freq.Width = 150;
            // 
            // columnHeader_DDCCPList_BW
            // 
            this.columnHeader_DDCCPList_BW.Text = "$DDC_CPLIST_BW";
            this.columnHeader_DDCCPList_BW.Width = 150;
            // 
            // columnHeader_DDCCPList_Gain
            // 
            this.columnHeader_DDCCPList_Gain.Text = "$DDC_CPLIST_GAIN";
            this.columnHeader_DDCCPList_Gain.Width = 150;
            // 
            // contextMenuStrip_DDCList
            // 
            this.contextMenuStrip_DDCList.ImageScalingSize = new System.Drawing.Size(32, 32);
            this.contextMenuStrip_DDCList.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripMenuItem_DDCCPList_Add,
            this.toolStripMenuItem_DDCCPList_Chg,
            this.toolStripMenuItem_DDCCPList_RM,
            this.toolStripMenuItem_DDCCPList_Clr});
            this.contextMenuStrip_DDCList.Name = "contextMenuStrip_DDCList";
            this.contextMenuStrip_DDCList.Size = new System.Drawing.Size(182, 92);
            this.contextMenuStrip_DDCList.Opening += new System.ComponentModel.CancelEventHandler(this.contextMenuStrip_DDCList_Opening);
            // 
            // toolStripMenuItem_DDCCPList_Add
            // 
            this.toolStripMenuItem_DDCCPList_Add.Name = "toolStripMenuItem_DDCCPList_Add";
            this.toolStripMenuItem_DDCCPList_Add.Size = new System.Drawing.Size(181, 22);
            this.toolStripMenuItem_DDCCPList_Add.Text = "$DDC_CPLIST_ADD";
            this.toolStripMenuItem_DDCCPList_Add.Click += new System.EventHandler(this.toolStripMenuItem_DDCCPList_Add_Click);
            // 
            // toolStripMenuItem_DDCCPList_Chg
            // 
            this.toolStripMenuItem_DDCCPList_Chg.Name = "toolStripMenuItem_DDCCPList_Chg";
            this.toolStripMenuItem_DDCCPList_Chg.Size = new System.Drawing.Size(181, 22);
            this.toolStripMenuItem_DDCCPList_Chg.Text = "$DDC_CPLIST_CHG";
            this.toolStripMenuItem_DDCCPList_Chg.Click += new System.EventHandler(this.toolStripMenuItem_DDCCPList_Chg_Click);
            // 
            // toolStripMenuItem_DDCCPList_RM
            // 
            this.toolStripMenuItem_DDCCPList_RM.Name = "toolStripMenuItem_DDCCPList_RM";
            this.toolStripMenuItem_DDCCPList_RM.Size = new System.Drawing.Size(181, 22);
            this.toolStripMenuItem_DDCCPList_RM.Text = "$DDC_CPLIST_RM";
            this.toolStripMenuItem_DDCCPList_RM.Click += new System.EventHandler(this.toolStripMenuItem_DDCCPList_RM_Click);
            // 
            // toolStripMenuItem_DDCCPList_Clr
            // 
            this.toolStripMenuItem_DDCCPList_Clr.Name = "toolStripMenuItem_DDCCPList_Clr";
            this.toolStripMenuItem_DDCCPList_Clr.Size = new System.Drawing.Size(181, 22);
            this.toolStripMenuItem_DDCCPList_Clr.Text = "$DDC_CPLIST_CLR";
            this.toolStripMenuItem_DDCCPList_Clr.Click += new System.EventHandler(this.toolStripMenuItem_DDCCPList_Clr_Click);
            // 
            // openFileDialog_DDCProject
            // 
            this.openFileDialog_DDCProject.DefaultExt = "vdcprj";
            this.openFileDialog_DDCProject.Filter = "ViPER-DDC Project (*.vdcprj)|*.vdcprj";
            this.openFileDialog_DDCProject.Title = "$DDC_OPEN_PROJECT";
            // 
            // saveFileDialog_DDCProject
            // 
            this.saveFileDialog_DDCProject.DefaultExt = "vdcprj";
            this.saveFileDialog_DDCProject.Filter = "ViPER-DDC Project (*.vdcprj)|*.vdcprj";
            this.saveFileDialog_DDCProject.Title = "$DDC_SAVE_PROJECT";
            // 
            // saveFileDialog_VDCData
            // 
            this.saveFileDialog_VDCData.DefaultExt = "vdc";
            this.saveFileDialog_VDCData.Filter = "ViPER-DDC (*.vdc)|*.vdc";
            this.saveFileDialog_VDCData.Title = "$DDC_EXPORT_VDC";
            // 
            // frmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.ClientSize = new System.Drawing.Size(715, 414);
            this.Controls.Add(this.tabControl_Main);
            this.Controls.Add(this.menuStrip_Main);
            this.DoubleBuffered = true;
            this.MainMenuStrip = this.menuStrip_Main;
            this.Margin = new System.Windows.Forms.Padding(2);
            this.MaximizeBox = false;
            this.Name = "frmMain";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "$MAINFORMTITLE";
            this.menuStrip_Main.ResumeLayout(false);
            this.menuStrip_Main.PerformLayout();
            this.tabControl_Main.ResumeLayout(false);
            this.tabPage_DDCMode.ResumeLayout(false);
            this.groupBox_DDCPoints.ResumeLayout(false);
            this.contextMenuStrip_DDCList.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();
        }

        private void listView_DDCPoints_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                this.contextMenuStrip_DDCList.Tag = "CPList";
                this.contextMenuStrip_DDCList.Show(this.listView_DDCPoints, e.Location);
            }
        }

        private void panel_DDCView_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                this.contextMenuStrip_DDCList.Tag = "DDCView";
                this.contextMenuStrip_DDCList.Show(this.panel_DDCView, e.Location);
            }
        }

        private void panel_DDCView_Paint(object sender, PaintEventArgs e)
        {
            Graphics graphics = e.Graphics;
            RectangleF ef = new RectangleF((float) this.panel_DDCView.ClientRectangle.X, (float) this.panel_DDCView.ClientRectangle.Y, (float) this.panel_DDCView.ClientRectangle.Width, (float) this.panel_DDCView.ClientRectangle.Height);
            graphics.CompositingQuality = CompositingQuality.HighQuality;
            graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
            graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;
            graphics.SmoothingMode = SmoothingMode.HighQuality;
            graphics.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
            graphics.Clear(Color.White);
            float[] responseTable = this.g_dcDDCContext.GetResponseTable(0x100, 44100.0);
            if (responseTable == null)
            {
                responseTable = new float[0x100];
                for (int j = 0; j < 0x100; j++)
                {
                    responseTable[j] = 0f;
                }
            }
            float num2 = 0f;
            for (int i = 0; i < responseTable.Length; i++)
            {
                if (Math.Abs(responseTable[i]) > num2)
                {
                    num2 = Math.Abs(responseTable[i]);
                }
            }
            if (num2 <= 1E-08f)
            {
                graphics.DrawLine(new Pen(Brushes.Blue, 1f), 0f, ef.Height / 2f, ef.Width, ef.Height / 2f);
            }
            else
            {
                if (num2 > 24f)
                {
                    float num4 = 24f / num2;
                    for (int n = 0; n < responseTable.Length; n++)
                    {
                        responseTable[n] *= num4;
                    }
                }
                for (int k = 0; k < responseTable.Length; k++)
                {
                    responseTable[k] /= 24f;
                }
                float num7 = ef.Width / ((float) responseTable.Length);
                float num8 = 0f;
                float num9 = ef.Height / 2f;
                for (int m = 0; m < responseTable.Length; m++)
                {
                    float num11 = (-responseTable[m] * (ef.Height / 2f)) + (ef.Height / 2f);
                    graphics.DrawLine(new Pen(Brushes.Blue, 1f), num8, num9, num8 + num7, num11);
                    num8 += num7;
                    num9 = num11;
                }
            }
        }

        private void RefreshMUI()
        {
            this.Text = this.g_mcMUIContent.MainFormTitle;
            this.toolStripMenuItem_Lan.Text = this.g_mcMUIContent.Menu_Language;
            this.toolStripMenuItem_LAN_eng.Text = this.g_mcMUIContent.Menu_Language_eng;
            this.toolStripMenuItem_LAN_zh_CN.Text = this.g_mcMUIContent.Menu_Language_zh_CN;
            this.toolStripMenuItem_LAN_zh_TW.Text = this.g_mcMUIContent.Menu_Language_zh_TW;
            this.tabPage_DDCMode.Text = this.g_mcMUIContent.DDCToolMode;
            this.groupBox_DDCPoints.Text = this.g_mcMUIContent.DDC_CalibrationPoints;
            this.listView_DDCPoints.Columns[0].Text = this.g_mcMUIContent.DDC_CPList_Freq;
            this.listView_DDCPoints.Columns[1].Text = this.g_mcMUIContent.DDC_CPList_BW;
            this.listView_DDCPoints.Columns[2].Text = this.g_mcMUIContent.DDC_CPList_Gain;
            this.toolStripMenuItem_DDCCPList_Add.Text = this.g_mcMUIContent.DDC_CPList_Menu_AddPoint;
            this.toolStripMenuItem_DDCCPList_Chg.Text = this.g_mcMUIContent.DDC_CPList_Menu_ChgPoint;
            this.toolStripMenuItem_DDCCPList_RM.Text = this.g_mcMUIContent.DDC_CPList_Menu_RMPoint;
            this.toolStripMenuItem_DDCCPList_Clr.Text = this.g_mcMUIContent.DDC_CPList_Menu_ClrPoint;
            this.button_OpenDDCProject.Text = this.g_mcMUIContent.DDC_Open_Project;
            this.button_SaveDDCProject.Text = this.g_mcMUIContent.DDC_Save_Project;
            this.button_DDCExportVDC.Text = this.g_mcMUIContent.DDC_Export_Data;
            this.openFileDialog_DDCProject.Title = this.g_mcMUIContent.DDC_Open_Project;
            this.saveFileDialog_DDCProject.Title = this.g_mcMUIContent.DDC_Save_Project;
            this.saveFileDialog_VDCData.Title = this.g_mcMUIContent.DDC_Export_Data;
        }

        private void toolStripMenuItem_DDCCPList_Add_Click(object sender, EventArgs e)
        {
            frmAddPoint point = new frmAddPoint(this.g_mcMUIContent);
            if (point.ShowDialog() == DialogResult.OK)
            {
                this.g_mtxDDCListMutex.WaitOne();
                int calibrationPointFrequency = point.CalibrationPointFrequency;
                double calibrationPointBandwidth = point.CalibrationPointBandwidth;
                double calibrationPointGain = point.CalibrationPointGain;
                for (int i = 0; i < this.listView_DDCPoints.Items.Count; i++)
                {
                    if (this.listView_DDCPoints.Items[i].Text == calibrationPointFrequency.ToString())
                    {
                        this.g_mtxDDCListMutex.ReleaseMutex();
                        MessageBox.Show(this.g_mcMUIContent.Tip_Text_Point_Already_Exists, this.g_mcMUIContent.MainFormTitle, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                        return;
                    }
                }
                ListViewItem item = new ListViewItem {
                    Text = calibrationPointFrequency.ToString()
                };
                item.SubItems.Add(calibrationPointBandwidth.ToString("F02"));
                item.SubItems.Add(calibrationPointGain.ToString("F02"));
                this.listView_DDCPoints.Items.Add(item);
                this.g_dcDDCContext.AddFilter(calibrationPointFrequency, calibrationPointGain, calibrationPointBandwidth, 44100.0);
                this.listView_DDCPoints.Sort();
                this.g_mtxDDCListMutex.ReleaseMutex();
                this.panel_DDCView.Invalidate();
            }
        }

        private void toolStripMenuItem_DDCCPList_Chg_Click(object sender, EventArgs e)
        {
            this.g_mtxDDCListMutex.WaitOne();
            if (this.listView_DDCPoints.SelectedItems == null)
            {
                this.g_mtxDDCListMutex.ReleaseMutex();
            }
            else if (this.listView_DDCPoints.SelectedItems.Count != 1)
            {
                this.g_mtxDDCListMutex.ReleaseMutex();
            }
            else
            {
                int result = 0;
                int nOldFreq = 0;
                double calibrationPointBandwidth = 0.0;
                double calibrationPointGain = 0.0;
                int.TryParse(this.listView_DDCPoints.SelectedItems[0].Text, out result);
                double.TryParse(this.listView_DDCPoints.SelectedItems[0].SubItems[1].Text, out calibrationPointBandwidth);
                double.TryParse(this.listView_DDCPoints.SelectedItems[0].SubItems[2].Text, out calibrationPointGain);
                nOldFreq = result;
                frmEditPoint point = new frmEditPoint(this.g_mcMUIContent) {
                    CalibrationPointFrequency = result,
                    CalibrationPointBandwidth = calibrationPointBandwidth,
                    CalibrationPointGain = calibrationPointGain
                };
                if (point.ShowDialog() == DialogResult.OK)
                {
                    result = point.CalibrationPointFrequency;
                    calibrationPointBandwidth = point.CalibrationPointBandwidth;
                    calibrationPointGain = point.CalibrationPointGain;
                    for (int i = 0; i < this.listView_DDCPoints.Items.Count; i++)
                    {
                        if ((this.listView_DDCPoints.Items[i].Index != this.listView_DDCPoints.SelectedItems[0].Index) && (this.listView_DDCPoints.Items[i].Text == result.ToString()))
                        {
                            this.g_mtxDDCListMutex.ReleaseMutex();
                            MessageBox.Show(this.g_mcMUIContent.Tip_Text_Point_Already_Exists, this.g_mcMUIContent.MainFormTitle, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                            return;
                        }
                    }
                    this.listView_DDCPoints.SelectedItems[0].Text = result.ToString();
                    this.listView_DDCPoints.SelectedItems[0].SubItems[1].Text = calibrationPointBandwidth.ToString("F02");
                    this.listView_DDCPoints.SelectedItems[0].SubItems[2].Text = calibrationPointGain.ToString("F02");
                    this.g_dcDDCContext.ModifyFilter(nOldFreq, result, calibrationPointGain, calibrationPointBandwidth, 44100.0);
                    this.listView_DDCPoints.Sort();
                }
                this.g_mtxDDCListMutex.ReleaseMutex();
                this.panel_DDCView.Invalidate();
            }
        }

        private void toolStripMenuItem_DDCCPList_Clr_Click(object sender, EventArgs e)
        {
            this.g_mtxDDCListMutex.WaitOne();
            this.listView_DDCPoints.Items.Clear();
            this.g_mtxDDCListMutex.ReleaseMutex();
            this.g_dcDDCContext.ClearFilters();
            this.panel_DDCView.Invalidate();
        }

        private void toolStripMenuItem_DDCCPList_RM_Click(object sender, EventArgs e)
        {
            this.g_mtxDDCListMutex.WaitOne();
            if (this.listView_DDCPoints.SelectedItems == null)
            {
                this.g_mtxDDCListMutex.ReleaseMutex();
            }
            else
            {
                bool flag = false;
                List<ListViewItem> list = new List<ListViewItem>();
                for (int i = 0; i < this.listView_DDCPoints.SelectedItems.Count; i++)
                {
                    list.Add(this.listView_DDCPoints.SelectedItems[i]);
                }
                for (int j = 0; j < list.Count; j++)
                {
                    int result = 0;
                    if (int.TryParse(list[j].Text, out result))
                    {
                        this.g_dcDDCContext.RemoveFilter(result);
                        this.listView_DDCPoints.Items.Remove(list[j]);
                    }
                }
                if (list.Count > 0)
                {
                    flag = true;
                }
                list.Clear();
                if (flag)
                {
                    this.listView_DDCPoints.Sort();
                }
                this.g_mtxDDCListMutex.ReleaseMutex();
                this.panel_DDCView.Invalidate();
            }
        }

        private void toolStripMenuItem_LAN_eng_Click(object sender, EventArgs e)
        {
            MUI.LoadResources("en-US", out this.g_mcMUIContent);
            this.RefreshMUI();
        }

        private void toolStripMenuItem_LAN_zh_CN_Click(object sender, EventArgs e)
        {
            MUI.LoadResources("zh-CN", out this.g_mcMUIContent);
            this.RefreshMUI();
        }

        private void toolStripMenuItem_LAN_zh_TW_Click(object sender, EventArgs e)
        {
            MUI.LoadResources("zh-TW", out this.g_mcMUIContent);
            this.RefreshMUI();
        }

        private class DDCPointsSorter : IComparer
        {
            private Comparer<int> m_Comparer = Comparer<int>.Default;

            public int Compare(object x, object y)
            {
                ListViewItem item = x as ListViewItem;
                ListViewItem item2 = y as ListViewItem;
                int result = 0;
                int num2 = 0;
                int.TryParse(item.Text, out result);
                int.TryParse(item2.Text, out num2);
                return this.m_Comparer.Compare(result, num2);
            }
        }
    }
}