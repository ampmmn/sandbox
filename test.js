fs = new ActiveXObject("Scripting.FileSystemObject");
wsh = new ActiveXObject("WScript.Shell");

TARGET_SHEET_NAME = 'sheet-name';
MAX_ROWS = 100;
MAX_COLUMNS = 100;
WORKSPACE = wsh.Environment('PROCESS').Item("WORKSPACE");

var baseFilePath = 'path/to/file.xlsx';

//var dstFilePath = WORKSPACE;
var dstFilePath = "C:\\workspace";

if (fs.FileExists(baseFilePath) == false) {
	WScript.Echo("baseFile: Not Found."); 
	WScript.Quit(1);
}


// 前回のタイムスタンプ
if (FileIsUpdated(baseFilePath) == false) {
	WScript.Echo("ファイルは更新されていません"); 
	WScript.Quit(0);
}

dstFilePath += "\\" + fs.GetFileName(baseFilePath);
WScript.Echo(dstFilePath);

try {
	// ファイルをローカルにコピーする
	fs.CopyFile(baseFilePath, dstFilePath);
}
catch(e) {
	WScript.Echo("ファイルのコピーに失敗しました");
	WScript.Echo(baseFilePath + "->" + dstFilePath);
	WScript.Quit(1);
}
	
var excel = null;
try {
	// ファイルを開く
	excel = new ActiveXObject("Excel.Application");
	excel.Visible = false;
	excel.ScreenUpdating = false;
	excel.EnableEvents = false;

	var wb = excel.WorkBooks.Open(dstFilePath);
	
	// 特定のシートのみを残して削除する
	for (var i = 1; i <= wb.WorkSheets.Count;) {
		var sheet = wb.WorkSheets(i);
		if (sheet.Name == TARGET_SHEET_NAME) {
			i++;
			continue;
		}
		sheet.Delete();
	}
	
	var cells = wb.WorkSheets(1).Cells;
	
	// 特定の文言を削除する
	for (var row = 1; row <= MAX_ROWS; ++row) {
		for (var col = 1; col <= MAX_COLUMNS; ++col) {
			var cell = cells.Item(row, col);

			var text = cell.text;

			text = text.replace(/[AE]\d\d\d[A-Z][A-Z]/g, '');
			text = text.replace(/[AE]\d\d\d[A-Z] ?(\/ ?[A-Z])?/g, '');
			text = text.replace(/[AE]\d\d\d/g, '');

			cell.value = text;
		}
	}
	
	// 保存する
	wb.Save();
	wb.Close(false);

	UpdateTimestamp(baseFilePath);

	excel.EnableEvents = true;
	excel.ScreenUpdating = true;
	excel.Quit();

	// 後段で更新したかどうかを判断するためのキーワード
	WScript.Echo("UPDATED");

	WScript.Quit(0);
}
catch(e) {
	WScript.Echo("An unexpected error occurred during xlsx processing.");

	if (excel) {
		excel.Quit();
	}

	WScript.Quit(1);
}


// ファイルが前回から更新されたかどうかを判断する
function FileIsUpdated(filePath)
{
	var fileObj = fs.GetFile(filePath);
	var curTimestamp = (new Date(fileObj.DateLastModified)).getTime();
	fileObj = null;

	var timestampPath += WORKSPACE + "\\timestamp.txt";

	if (fs.FileExists(timestampPath) == false) {
		return true;
	}

	var fileTime = fs.OpenTextFile(timestampPath, 1, false);
	var lastTimestamp = parseInt(fileTime.ReadLine());
	fileTime.Close();

	return lastTimestamp < curTimestamp;
}

// ファイルのタイムスタンプ情報を更新する
function UpdateTimestamp(filePath)
{
	var fileObj = fs.GetFile(filePath);
	var curTimestamp = (new Date(fileObj.DateLastModified)).getTime();
	fileObj = null;

	var timestampPath += WORKSPACE + "\\timestamp.txt";

	var fileTime = fs.CreateTextFile(timestampPath);
	fileTime.WriteLine(curTimestamp);
	fileTime.Close();
}

