// 2017.04.04
///////////////////////  Создание структуры подкаста  /////////////////////////
#define mpiJsonInfo 40032 // Идентификатор для хранения json информации о фильме
#define mpiKPID     40033 // Идентификатор для хранения ID кинопоиска

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
string    gsUrlBase    = "http://hdkinoteatr.com"; 
int       gnTotalItems = 0; 
TDateTime gStart       = Now;
string    gsAPIUrl     = "http://api.lostcut.net/hdkinoteatr/";

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

///////////////////////////////////////////////////////////////////////////////
// Установка переменной Podcast: поиск родительской папки, содержащий скрипт
THmsScriptMediaItem GetRoot() {
  Podcast = FolderItem; // Начиная с текущего элемента, ищется создержащий срипт
  while ((Trim(Podcast[550])=='') && (Podcast.ItemParent!=nil)) Podcast=Podcast.ItemParent;
  return Podcast;
}

///////////////////////////////////////////////////////////////////////////////
// Создание папки или подкаста
THmsScriptMediaItem CreateFolder(THmsScriptMediaItem Parent, string sName, string sLink, string sParams='', bool bForceFolder=false, bool bCreateStruct=false) {
  THmsScriptMediaItem Item = Parent.AddFolder(sLink, bForceFolder); // Создаём папку с указанной ссылкой
  Item[mpiTitle     ] = sName; // Присваиваем наименование
  Item[mpiCreateDate] = IncTime(gStart,0,-gnTotalItems,0,0); gnTotalItems++;
  Item[mpiPodcastParameters] = sParams;
  Item[mpiFolderSortOrder  ] = "-mpCreateDate";
  if (bCreateStruct) Item[570] = 2;
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Функция создания динамической папки с указанным скриптом
THmsScriptMediaItem CreateDynamicItem(THmsScriptMediaItem prntItem, string sTitle, string sLink, string &sScript='') {
  THmsScriptMediaItem Folder = prntItem.AddFolder(sLink, false, 32);
  Folder[mpiTitle     ] = sTitle;
  Folder[mpiCreateDate] = VarToStr(IncTime(Now,0,-prntItem.ChildCount,0,0));
  Folder[200] = 5;           // mpiFolderType
  Folder[500] = sScript;     // mpiDynamicScript
  Folder[501] = 'C++Script'; // mpiDynamicSyntaxType
  Folder[mpiFolderSortOrder] = -mpiCreateDate;
  return Folder;
}

///////////////////////////////////////////////////////////////////////////////
// Замена в тексте загруженного скрипта значения текстовой переменной
void ReplaceVarValue(string &sText, string sVarName, string sNewVal) {
  string sVal, sVal2;
  if (HmsRegExMatch2("("+sVarName+"\\s*?=.*?';)", sText, sVal, sVal2)) {
    HmsRegExMatch(sVarName+"\\s*?=\\s*?'(.*)'", sVal, sVal2);
    sText = ReplaceStr(sText, sVal, ReplaceStr(sVal , sVal2, sNewVal));
  }
}

///////////////////////////////////////////////////////////////////////////////
// Создание папки ПОИСК (с загрузкой скрипта с форума homemediaserver.ru)
void CreateSearchFolder(THmsScriptMediaItem Parent, string sTitle) {
  string sScript='', sLink, sHtml, sRE, sVal; THmsScriptMediaItem Folder;
  
  // Да да, загружаем скрипт с сайта форума HMS
  sHtml = HmsUtf8Decode(HmsDownloadURL('http://homemediaserver.ru/forum/viewtopic.php?f=15&t=2793&p=17395#p17395', '', true));
  HmsRegExMatch('BeginDynamicSearchScript\\*/(.*?)/\\*EndDynamicSearchScript', sHtml, sScript, 1, PCRE_SINGLELINE);
  sScript = HmsHtmlToText(sScript, 1251);
  sScript = ReplaceStr(sScript, #160, ' ');
  
  // И меняем значения переменных на свои
  ReplaceVarValue(sScript, 'gsSuggestQuery'  , gsAPIUrl+'videos?q=');
  ReplaceVarValue(sScript, 'gsSuggestRegExpr', '"name":"(.*?)"');
  ReplaceVarValue(sScript, 'gsSuggestMethod' , 'GET');
  //sScript = ReplaceStr(sScript, 'gnSuggestNoUTFEnc = 0', 'gnSuggestNoUTFEnc = 1');
  
  Folder = Parent.AddFolder(sTitle, true);
  Folder[mpiCreateDate     ] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0));
  Folder[mpiFolderSortOrder] = "-mpCreateDate";
  gnTotalItems++;
  
  CreateDynamicItem(Folder, '"Набрать текст"', '-SearchCommands', sScript);
  
  CreateFolder(Folder, 'Режиссёры' , 'directors' , '', true);
  CreateFolder(Folder, 'Актёры'    , 'actors'    , '', true);
  CreateFolder(Folder, 'Сценаристы', 'scenarists', '', true);
  CreateFolder(Folder, 'Продюсеры' , 'producers' , '', true);
}

///////////////////////////////////////////////////////////////////////////////
// Получение название группы из имени
string GetGroupName(string sName) {
  string sGrp = '#';
  if (HmsRegExMatch('([A-ZА-Я0-9])', sName, sGrp, 1, PCRE_CASELESS)) sGrp = Uppercase(sGrp);
  if (HmsRegExMatch('[0-9]', sGrp, sGrp)) sGrp = '#';
  if (HmsRegExMatch('[A-Z]', sGrp, sGrp)) sGrp = 'A..Z';
  return sGrp;
}

///////////////////////////////////////////////////////////////////////////////
// Создание папки с категориями
void CreateCategories(THmsScriptMediaItem Parent, string sName, string sLink, string sParams='') {
  string sData, sVal, sRes; int i; TJsonObject JSON, VIDEO; TJsonArray JARRAY;
  THmsScriptMediaItem Folder;
  
  Folder = CreateFolder(Parent, sName, sLink, sParams, true);
  if      (Pos('category', sLink)>0) sRes = 'categories';
  else if (Pos('country' , sLink)>0) sRes = 'countries';
  else return;
  
  if (Pos('serials=1', sLink)>0) sRes += '?serials=1';
  
  sData = HmsDownloadURL(gsAPIUrl+sRes, "Accept-Encoding: gzip, deflate", true);
  JSON  = TJsonObject.Create();
  try {
    JSON.LoadFromString(sData);
    JARRAY = JSON.AsArray; if (JARRAY==nil) return;
    for (i=0; i<JARRAY.Length; i++) {
      VIDEO = JARRAY[i];
      CreateFolder(Folder, HmsHtmlToText(VIDEO.S['name'], 65001), sLink+'='+VIDEO.S['id']);
    }
  } finally { JSON.Free; }
}

///////////////////////////////////////////////////////////////////////////////
// Создание папки с подкастами годов
void CreateYears(THmsScriptMediaItem Parent, string sName, string sLink) {
  int i, nCurrentYear, nCurrentMonth, nCurrentDay; THmsScriptMediaItem Folder;
  
  Folder = CreateFolder(Parent, sName, sLink, '', true);
  // Узнаём начальное значение года, с которого стоит начинать создавать список
  DecodeDate(Now, nCurrentYear, nCurrentMonth, nCurrentDay);
  if (nCurrentMonth > 10) nCurrentYear++;
  
  for(i=nCurrentYear; i >= 1995; i--) {
    CreateFolder(Folder, Str(i), sLink+'='+Str(i));
  }
}

///////////////////////////////////////////////////////////////////////////////
// Удаление существующих разделов (перед созданием)
bool DeleteFolders() {
  THmsScriptMediaItem Item, FavFolder; int i, nAnsw;
  FavFolder = HmsFindMediaFolder(FolderItem.ItemID, 'favorites');
  if (FavFolder==nil) { FolderItem.DeleteChildItems(); return true; }
  nAnsw = MessageDlg('Очистить папку "Избранное"?', mtConfirmation, mbYes+mbNo+mbCancel, 0);
  if (nAnsw== mrCancel) return false;
  for (i=FolderItem.ChildCount-1; i>=0; i--) {
    Item = FolderItem.ChildItems[i];
    if ((Item==FavFolder) && (nAnsw==mrNo)) continue;
    Item.Delete();
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Создание списка сериалов
void CreateSerials() {
  string sData, sName, sLink, sID, sKPID, sImg, sGrp, sPrevGrp=''; int i; TJsonObject JSON, VIDEO; TJsonArray JARRAY;
  THmsScriptMediaItem Folder=FolderItem, Serial, Group;
  
  sData = HmsDownloadURL(gsAPIUrl+'videos?serials=1&limit=9000&ord=name', "Accept-Encoding: gzip, deflate", true);
  JSON  = TJsonObject.Create();
  try {
    JSON.LoadFromString(sData);
    JARRAY = JSON.AsArray; if (JARRAY==nil) return;
    for (i=0; i<JARRAY.Length; i++) {
      VIDEO = JARRAY[i];
      sName = HmsHtmlToText(VIDEO.S['name'], 65001);
      sID   = VIDEO.S['id'  ];
      sKPID = VIDEO.S['kpid'];
      sImg  = '';
      if (Length(sKPID)>1) sImg = 'https://st.kp.yandex.net/images/film_iphone/iphone360_'+sKPID+'.jpg';
      sGrp  = GetGroupName(sName);
      if (sPrevGrp!=sGrp) {
        Group = Folder.AddFolder(sGrp, true);
        Group[mpiCreateDate     ] = IncTime(gStart,0,-gnTotalItems,0,0);
        Group[mpiFolderSortOrder] = "mpTitle";
      }
      Serial = CreateFolder(Group, sName, VIDEO.S['page']);
      Serial[mpiThumbnail] = sImg;
      Serial[mpiJsonInfo ] = VIDEO.SaveToString();
      Serial[mpiKPID     ] = sKPID;
      sPrevGrp = sGrp;
    }
  } finally { JSON.Free; }
  Folder.Sort("mpTitle");
}

///////////////////////////////////////////////////////////////////////////////
//                     Г Л А В Н А Я   П Р О Ц Е Д У Р А                     //
{
  THmsScriptMediaItem Folder;
  
  if (!DeleteFolders()) return;

  if (FolderItem==Podcast) {
    // Это корневая папка подкаста - создём структуру
    CreateFolder      (FolderItem, '00 Избранное'         , 'favorites'  , '', true);
    CreateSearchFolder(FolderItem, '01 Поиск');
    CreateFolder      (FolderItem, '02 Последние поступления', 'serials=0');
    CreateFolder      (FolderItem, '03 Сериалы'           , 'serials', '', true, true);
    CreateCategories  (FolderItem, '04 Категории'         , 'serials=0&limit=500&category');
    CreateYears       (FolderItem, '05 По-годам'          , 'serials=0&limit=500&year');
    CreateCategories  (FolderItem, '06 По-странам'        , 'serials=0&country', '--group=year');
    CreateFolder      (FolderItem, '07 TOP250 IMDb'       , 'top=imdb');
    CreateFolder      (FolderItem, '08 TOP250 Kinopoisk'  , 'top=kp'  );
    CreateFolder      (FolderItem, '09 TOP250 HDKinoteatr', 'top=hd'  );
    
    Folder = CreateFolder(FolderItem, '10 С рейтингом IMDb от 6.0', 'imdb>6', '', true);
    CreateFolder    (Folder, '1 Последние поступления IMDb>6', 'serials=0&min_imdb=6');
    CreateCategories(Folder, '2 Категории'      , 'serials=0&min_imdb=6&limit=500&category');
    CreateYears     (Folder, '3 По-годам'       , 'serials=0&min_imdb=6&limit=500&year');
    CreateCategories(Folder, '4 По-странам'     , 'serials=0&min_imdb=6&country', '--group=year');
    
    Folder = CreateFolder(FolderItem, '11 С рейтингом KP от 6.0', 'kp>6', '', true);
    CreateFolder    (Folder, '1 Последние поступления KP>6', 'serials=0&min_kp=6');
    CreateCategories(Folder, '2 Категории'      , 'serials=0&min_kp=6&limit=500&category');
    CreateYears     (Folder, '3 По-годам'       , 'serials=0&min_kp=6&limit=500&year');
    CreateCategories(Folder, '4 По-странам'     , 'serials=0&min_kp=6&country', '--group=year');
    
    Folder = CreateFolder(FolderItem, '12 С рейтингом HD от 6.0', 'hd>6', '', true);
    CreateFolder    (Folder, '1 Последние поступления HD>6', 'serials=0&min_hd=6');
    CreateCategories(Folder, '2 Категории'      , 'serials=0&min_hd=6&limit=500&category');
    CreateYears     (Folder, '3 По-годам'       , 'serials=0&min_hd=6&limit=500&year');
    CreateCategories(Folder, '4 По-странам'     , 'serials=0&min_hd=6&country', '--group=year');
    
  } else if (mpFilePath=='serials') {
    // Создание списка сериалов - отдельно
    CreateSerials();
    
  }
  
  HmsLogMessage(1, mpTitle+': Создано элементов - '+IntToStr(gnTotalItems));
}
