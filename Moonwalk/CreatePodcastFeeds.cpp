// VERSION = 2018.03.23
///////////////////////  Создание структуры подкаста  /////////////////////////
#define mpiJsonInfo 40032 // Идентификатор для хранения json информации о фильме
#define mpiKPID     40033 // Идентификатор для хранения ID кинопоиска

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
string gsUrlBase="http://moonwalk.co"; int gnTotalItems=0; TDateTime gTimeStart=Now;

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

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
void CreateSearchFolder(THmsScriptMediaItem prntItem, string sTitle) {
  string sScript='', sLink, sHtml, sRE, sVal; THmsScriptMediaItem Folder;
  
  // Да да, загружаем скрипт с сайта форума HMS
  sHtml = HmsUtf8Decode(HmsDownloadURL('http://homemediaserver.ru/forum/viewtopic.php?f=15&t=2793&p=17395#p17395', '', true));
  HmsRegExMatch('BeginDynamicSearchScript\\*/(.*?)/\\*EndDynamicSearchScript', sHtml, sScript, 1, PCRE_SINGLELINE);
  sScript = HmsHtmlToText(sScript, 1251);
  sScript = ReplaceStr(sScript, #160, ' ');
  
  // И меняем значения переменных на свои
  ReplaceVarValue(sScript, 'gsSuggestQuery'  , gsUrlBase+'/moonwalk/search?search_for=&commit=%D0%9D%D0%B0%D0%B9%D1%82%D0%B8&sq=');
  ReplaceVarValue(sScript, 'gsSuggestRegExpr', '<tr>\\\\s*?<td>(.*?)</td>');
  //ReplaceVarValue(sScript, 'gsSuggestMethod' , 'POST');
  //sScript = ReplaceStr(sScript, 'gnSuggestNoUTFEnc = 0', 'gnSuggestNoUTFEnc = 1');

  Folder = prntItem.AddFolder(sTitle, true);
  Folder[mpiCreateDate     ] = VarToStr(IncTime(gTimeStart,0,-gnTotalItems,0,0));
  Folder[mpiFolderSortOrder] = "-mpCreateDate";
  gnTotalItems++;

  CreateDynamicItem(Folder, '"Набрать текст"', '-SearchCommands', sScript);
}

///////////////////////////////////////////////////////////////////////////////
// Создание подкаста или папки
THmsScriptMediaItem CreateItem(THmsScriptMediaItem Parent, string sTitle="", string sLink="", string sImg="") {
  THmsScriptMediaItem Item; bool bForceFolder = false;

  if (sLink=='') { sLink = sTitle; bForceFolder = true; }
  else             sLink = HmsExpandLink(sLink, gsUrlBase);

  Item = Parent.AddFolder(sLink, bForceFolder);
  Item[mpiTitle     ] = sTitle;
  Item[mpiCreateDate] = VarToStr(IncTime(gTimeStart,0,-gnTotalItems,0,0));
  Item[mpiFolderSortOrder] = -mpiCreateDate;
  Item[mpiThumbnail ] = sImg;
  gnTotalItems++;
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Получение имени группировки по имени видео (первая буква, "A..Z" или "#")
string GetGroupName(string sName) {
  string sGrp = '#';
  if (HmsRegExMatch('([A-ZА-Я0-9])', sName, sGrp, 1, PCRE_CASELESS)) sGrp = UpperCase(sGrp);
  if (HmsRegExMatch('[A-Z]', sGrp, sGrp)) sGrp = 'A..Z';
  if (HmsRegExMatch('[0-9]', sGrp, sGrp)) sGrp = '#';
  return sGrp;
}

///////////////////////////////////////////////////////////////////////////////
// Создание списка сериалов
void CreateSeriesList() {
  string sHtml, sLink, sName, sYear, sKPID, sImg, sVal, sTran, sGroupMode;
  TRegExpr RE; THmsScriptMediaItem Item, Folder; bool bGroup = false;
  int i, n, nPages=0, iCnt=0, nGrp=0, nMaxPages=10, nMaxInGroup=100, nMinInGroup=100;
  
  // Проверка установленных дополнительных параметров
  if (HmsRegExMatch('--maxingroup=(\\d+)', mpPodcastParameters, sVal)) nMaxInGroup = StrToInt(sVal);
  
  HmsRegExMatch('--group=(\\w+)', mpPodcastParameters, sGroupMode);
  bool bYearInTitle = (Pos('--yearintitle', mpPodcastParameters)>0);

  sHtml = HmsUtf8Decode(HmsDownloadURL(mpFilePath));
  if (HmsRegExMatch('(<link.*?>)', sHtml, sVal)) sHtml = ReplaceStr(sHtml, sVal, '');
  if (HmsRegExMatch('(<meta.*?>)', sHtml, sVal)) sHtml = ReplaceStr(sHtml, sVal, '');
  
  nPages = WordCount(sHtml, "\n");
  i = 0;
  
  RE = TRegExpr.Create('(.*?)<br>', PCRE_SINGLELINE);
  try {
    if (RE.Search(sHtml)) do {
      i++; sLink=""; sName=""; sYear=""; sImg=""; sKPID=""; sTran="";
      
      if (i % 50 == 0) {
        HmsSetProgress(Int(i / nPages * 100));
        HmsShowProgress(IntToStr(i)+" из "+IntToStr(nPages));
        if (HmsCancelPressed) break;
      }
  
      HmsRegExMatch('^(.*?);'          , RE.Match, sName); // Название
      HmsRegExMatch('src="(.*?)"'      , RE.Match, sLink); // Ссылка
      HmsRegExMatch('^.*?;(\\d{4})'    , RE.Match, sYear); // Год
      HmsRegExMatch('^.*?;.*?;(.*?);'  , RE.Match, sKPID); // Код фильма на Kinopoisk
      HmsRegExMatch('iframe&gt;;(.*?);', RE.Match, sTran); // Озвучка / Перевод
      
      if (LeftCopy(sLink, 2)=="//") sLink = "http:"+Trim(sLink);
      else                          sLink = HmsExpandLink(sLink, gsUrlBase);
      HmsRegExReplace('(.*?moonwalk.)(co|pw)(.*)', sLink, '$1cc$3', sLink); // Волшебная заменялка moonwalk.pw на moonwalk.cc
      
      sName = Trim(HmsRemoveLineBreaks(HmsHtmlToText(sName)));
      
      if (sTran=='Не указан') sTran = '';
      
      if ((sKPID!='') && (sKPID!='0')) sImg = 'http://www.kinopoisk.ru/images/film/'+sKPID+'.jpg';
      
      if (sTran!='') sName += ' ['+sTran+']';
      if (bYearInTitle && (sYear!='') && (Pos(sYear, sName)<1)) sName += ' ('+sYear+')';
      
      // Контроль группировки (создаём папку с именем группы)
      if (sGroupMode=='alph') {
        Folder = FolderItem.AddFolder(GetGroupName(sName), true);
        Folder[mpiFolderSortOrder] = "mpTitle";
      } else if (sGroupMode=='year') {
        Folder = FolderItem.AddFolder(sYear, true);
        Folder[mpiFolderSortOrder] = "mpTitle";
        Folder[mpiYear           ] = sYear;
      } else if (bGroup) {
        iCnt++; if (iCnt>=nMaxInGroup) { nGrp++; iCnt=0; }
        Folder = FolderItem.AddFolder(Format('%.2d', [nGrp]), true);
      }
      
      Item = CreateItem(Folder, sName, sLink, sImg);        // Создание папки с фильмом
      Item[100500] = sKPID;
      
    } while (RE.SearchAgain);
    
  } finally { RE.Free(); HmsHideProgress(); }
  
  // Сортируем в базе данных программы созданные элементы
  if (sGroupMode=='alph') {
    FolderItem.Sort('mpTitle');
    for (i=0; i<FolderItem.ChildCount; i++) FolderItem.ChildItems[i].Sort('mpTitle');
  } else if (sGroupMode=='year') FolderItem.Sort('-mpYear');
  
  HmsLogMessage(1, mpTitle+': Создано ссылок - '+IntToStr(gnTotalItems));
}

///////////////////////////////////////////////////////////////////////////////
// ---------------------  M A I N  P R O C E D U R E  ---------------------- //
///////////////////////////////////////////////////////////////////////////////
{
  THmsScriptMediaItem Folder, Item;

  FolderItem.DeleteChildItems();
  
  if (RightCopy(mpFilePath, 4)==".txt") {
    CreateSeriesList();
    return;
  }

  Folder = CreateItem(FolderItem, '00. Избранное');
  CreateSearchFolder (FolderItem, '01. Поиск');
  Folder = CreateItem(FolderItem, '02. Новинки фильмов', '/moonwalk/search_as?sq=&kinopoisk_id=&search_for=film&search_year=updates&commit=%D0%9D%D0%B0%D0%B9%D1%82%D0%B8');
  Folder[mpiPodcastParameters] = '--maxpages=4 --maxingroup=160';
  Folder[mpiComment          ] = '--update';
  
  Folder = CreateItem(FolderItem, '03. По годам');
  Folder[mpiPodcastParameters] = '--maxpages=10';
  Folder[mpiFolderSortOrder  ] = mpiTitle;
  CreateItem(Folder, '2017'     , "/moonwalk/search_as?search_for=film&search_year=2017");
  CreateItem(Folder, '2016'     , "/moonwalk/search_as?search_for=film&search_year=2016");
  CreateItem(Folder, '2015'     , "/moonwalk/search_as?search_for=film&search_year=2015");
  CreateItem(Folder, '2014'     , "/moonwalk/search_as?search_for=film&search_year=2014");
  CreateItem(Folder, '2013'     , "/moonwalk/search_as?search_for=film&search_year=2013");
  CreateItem(Folder, '2012'     , "/moonwalk/search_as?search_for=film&search_year=2012");
  CreateItem(Folder, '2011'     , "/moonwalk/search_as?search_for=film&search_year=2011");
  CreateItem(Folder, '2010'     , "/moonwalk/search_as?search_for=film&search_year=2010");
  
  Folder = FolderItem.AddFolder(gsUrlBase+"/tv_show.txt", true);
  Folder[mpiTitle            ] = '04. Сериалы';
  Folder[mpiPodcastParameters] = '--group=alph';
  Folder[mpiFolderSortOrder  ] = mpiTitle;
  Folder[570] = 2; // Наследовать функцию создания лент подкастов
  
  //Folder = CreateItem(FolderItem, '04. Сериалы', '/tv_show.txt');
  //Folder[mpiPodcastParameters] = '--group=alph';
  //Folder[mpiFolderSortOrder  ] = mpiTitle;
  
  Folder = CreateItem(FolderItem, '05. Аниме', '/anime.txt');
  Folder[mpiPodcastParameters] = '--group=alph';
  Folder[mpiFolderSortOrder  ] = mpiTitle;
  
  Folder = CreateItem(FolderItem, '06. Все фильмы', '/film.txt');
  Folder[mpiPodcastParameters] = '--group=alph';
  Folder[mpiFolderSortOrder  ] = mpiTitle;
  
  HmsLogMessage(1, mpTitle+': Создано ссылок - '+IntToStr(gnTotalItems));
}
