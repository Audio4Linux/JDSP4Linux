namespace ViPERToolBox
{
    using System;
    using System.Runtime.InteropServices;

    public class MUI
    {
        public static void LoadResources(string szRegion, out MUIContent mcContent)
        {
            switch (szRegion.ToLower())
            {
                case "zh-cn":
                    LoadResources_zh_CN(out mcContent);
                    return;

                case "zh-tw":
                    LoadResources_zh_TW(out mcContent);
                    return;
            }
            LoadResources_eng(out mcContent);
        }

        private static void LoadResources_eng(out MUIContent mcContent)
        {
            mcContent = new MUIContent();
            mcContent.MainFormTitle = "ViPER DDC ToolBox: James34602 @ 2018";
            mcContent.Menu_Language = "Language";
            mcContent.Menu_Language_eng = "English";
            mcContent.Menu_Language_zh_CN = "Simplified Chinese";
            mcContent.Menu_Language_zh_TW = "Traditional Chinese";
            mcContent.DDCToolMode = "ViPER DDC Tools";
            mcContent.DDC_CalibrationPoints = "Calibration Points";
            mcContent.DDC_CPList_Freq = "Frequency";
            mcContent.DDC_CPList_BW = "Bandwidth";
            mcContent.DDC_CPList_Gain = "Gain";
            mcContent.DDC_CPList_Menu_AddPoint = "Add Point";
            mcContent.DDC_CPList_Menu_ChgPoint = "Edit Point";
            mcContent.DDC_CPList_Menu_RMPoint = "Remove Point(s)";
            mcContent.DDC_CPList_Menu_ClrPoint = "Clear All Points";
            mcContent.DDC_Open_Project = "Open DDC Project";
            mcContent.DDC_Save_Project = "Save DDC Project";
            mcContent.DDC_Export_Data = "Export to VDC";
            mcContent.AddPointFormTitle = "Add Calibration Point";
            mcContent.EditPointFormTitle = "Edit Calibration Point";
            mcContent.Tip_Text_OK = "OKay";
            mcContent.Tip_Text_Cancel = "Cancel";
            mcContent.Tip_Text_Invalid_Frequency = "Invalid frequency value";
            mcContent.Tip_Text_Invalid_Bandwidth = "Invalid bandwidth value";
            mcContent.Tip_Text_Invalid_Gain = "Invalid gain value";
            mcContent.Tip_Text_Point_Already_Exists = "Calibration frequency already exists";
            mcContent.Tip_Text_Failed_To_Export_VDC = "Failed to export VDC, invalid calibration point(s) found";
        }

        private static void LoadResources_zh_CN(out MUIContent mcContent)
        {
            mcContent = new MUIContent();
            mcContent.MainFormTitle = "ViPER音频工具: James34602 @ 2018";
            mcContent.Menu_Language = "语言";
            mcContent.Menu_Language_eng = "英语";
            mcContent.Menu_Language_zh_CN = "简体中文";
            mcContent.Menu_Language_zh_TW = "繁体中文";
            mcContent.DDCToolMode = "ViPER-DDC工具";
            mcContent.DDC_CalibrationPoints = "校准点";
            mcContent.DDC_CPList_Freq = "频率";
            mcContent.DDC_CPList_BW = "带宽";
            mcContent.DDC_CPList_Gain = "增益";
            mcContent.DDC_CPList_Menu_AddPoint = "添加校准点";
            mcContent.DDC_CPList_Menu_ChgPoint = "编辑校准点";
            mcContent.DDC_CPList_Menu_RMPoint = "删除校准点";
            mcContent.DDC_CPList_Menu_ClrPoint = "清空所有校准点";
            mcContent.DDC_Open_Project = "打开DDC工程";
            mcContent.DDC_Save_Project = "保存DDC工程";
            mcContent.DDC_Export_Data = "导出为VDC文件";
            mcContent.AddPointFormTitle = "添加校准点";
            mcContent.EditPointFormTitle = "编辑校准点";
            mcContent.Tip_Text_OK = "确定";
            mcContent.Tip_Text_Cancel = "取消";
            mcContent.Tip_Text_Invalid_Frequency = "无效的频率值";
            mcContent.Tip_Text_Invalid_Bandwidth = "无效的带宽值";
            mcContent.Tip_Text_Invalid_Gain = "无效的增益值";
            mcContent.Tip_Text_Point_Already_Exists = "校准的频点已经存在";
            mcContent.Tip_Text_Failed_To_Export_VDC = "无法导出VDC，发现无效的校准点";
        }

        private static void LoadResources_zh_TW(out MUIContent mcContent)
        {
            mcContent = new MUIContent();
            mcContent.MainFormTitle = "ViPER音訊工具: James34602 @ 2018";
            mcContent.Menu_Language = "語言";
            mcContent.Menu_Language_eng = "英語";
            mcContent.Menu_Language_zh_CN = "簡體中文";
            mcContent.Menu_Language_zh_TW = "繁體中文";
            mcContent.DDCToolMode = "ViPER-DDC工具";
            mcContent.DDC_CalibrationPoints = "校準點";
            mcContent.DDC_CPList_Freq = "頻點";
            mcContent.DDC_CPList_BW = "帶寬";
            mcContent.DDC_CPList_Gain = "增益";
            mcContent.DDC_CPList_Menu_AddPoint = "添加校準點";
            mcContent.DDC_CPList_Menu_ChgPoint = "編輯校準點";
            mcContent.DDC_CPList_Menu_RMPoint = "刪除校準點";
            mcContent.DDC_CPList_Menu_ClrPoint = "清空所有校準點";
            mcContent.DDC_Open_Project = "打開DDC檔案";
            mcContent.DDC_Save_Project = "保存DDC專案";
            mcContent.DDC_Export_Data = "輸出為VDC檔案";
            mcContent.AddPointFormTitle = "添加校準點";
            mcContent.EditPointFormTitle = "編輯校準點";
            mcContent.Tip_Text_OK = "確定";
            mcContent.Tip_Text_Cancel = "取消";
            mcContent.Tip_Text_Invalid_Frequency = "無效的頻率值";
            mcContent.Tip_Text_Invalid_Bandwidth = "無效的帶寬值";
            mcContent.Tip_Text_Invalid_Gain = "無效的增益值";
            mcContent.Tip_Text_Point_Already_Exists = "校準的頻點已經存在";
            mcContent.Tip_Text_Failed_To_Export_VDC = "無法輸出VDC，發現無效的校準點";
        }

        public class MUIContent
        {
            public string AddPointFormTitle;
            public string DDC_CalibrationPoints;
            public string DDC_CPList_BW;
            public string DDC_CPList_Freq;
            public string DDC_CPList_Gain;
            public string DDC_CPList_Menu_AddPoint;
            public string DDC_CPList_Menu_ChgPoint;
            public string DDC_CPList_Menu_ClrPoint;
            public string DDC_CPList_Menu_RMPoint;
            public string DDC_Export_Data;
            public string DDC_Open_Project;
            public string DDC_Save_Project;
            public string DDCToolMode;
            public string EditPointFormTitle;
            public string MainFormTitle;
            public string Menu_Language;
            public string Menu_Language_eng;
            public string Menu_Language_zh_CN;
            public string Menu_Language_zh_TW;
            public string Menu_View;
            public string Tip_Text_Cancel;
            public string Tip_Text_Failed_To_Export_VDC;
            public string Tip_Text_Invalid_Bandwidth;
            public string Tip_Text_Invalid_Frequency;
            public string Tip_Text_Invalid_Gain;
            public string Tip_Text_OK;
            public string Tip_Text_Point_Already_Exists;
        }
    }
}