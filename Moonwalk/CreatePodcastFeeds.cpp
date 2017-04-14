// VERSION = 2017.04.14
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
THmsScriptMediaItem CreateDynamicItem(THmsScriptMediaItem prntItem, char sTitle, char sLink, char &sScript='') {
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
void ReplaceVarValue(char &sText, char sVarName, char sNewVal) {
  char sVal, sVal2;
  if (HmsRegExMatch2("("+sVarName+"\\s*?=.*?';)", sText, sVal, sVal2)) {
    HmsRegExMatch(sVarName+"\\s*?=\\s*?'(.*)'", sVal, sVal2);
    sText = ReplaceStr(sText, sVal, ReplaceStr(sVal , sVal2, sNewVal));
  }
}

///////////////////////////////////////////////////////////////////////////////
// Создание папки ПОИСК (с загрузкой скрипта с форума homemediaserver.ru)
void CreateSearchFolder(THmsScriptMediaItem prntItem, char sTitle) {
  char sScript='', sLink, sHtml, sRE, sVal; THmsScriptMediaItem Folder;
  
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
THmsScriptMediaItem CreateItem(THmsScriptMediaItem Parent, char sTitle='', char sLink='') {
  THmsScriptMediaItem Item; bool bForceFolder = false;

  if (sLink=='') { sLink = sTitle; bForceFolder = true; }
  else             sLink = HmsExpandLink(sLink, gsUrlBase);

  Item = Parent.AddFolder(sLink, bForceFolder);
  Item[mpiTitle     ] = sTitle;
  Item[mpiCreateDate] = VarToStr(IncTime(gTimeStart,0,-gnTotalItems,0,0));
  Item[mpiFolderSortOrder] = -mpiCreateDate;
  gnTotalItems++;
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// ---------------------  M A I N  P R O C E D U R E  ---------------------- //
///////////////////////////////////////////////////////////////////////////////
{
  THmsScriptMediaItem Folder, Item;

  FolderItem.DeleteChildItems();

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
  
  Folder = CreateItem(FolderItem, '04. Сериалы', '/tv_show.txt');
  Folder[mpiPodcastParameters] = '--group=alph';
  Folder[mpiFolderSortOrder  ] = mpiTitle;
  
  Folder = CreateItem(FolderItem, '05. Аниме', '/anime.txt');
  Folder[mpiPodcastParameters] = '--group=alph';
  Folder[mpiFolderSortOrder  ] = mpiTitle;
  
  Folder = CreateItem(FolderItem, '06. Все фильмы', '/film.txt');
  Folder[mpiPodcastParameters] = '--group=alph';
  Folder[mpiFolderSortOrder  ] = mpiTitle;
  
  HmsLogMessage(1, mpTitle+': Создано ссылок - '+IntToStr(gnTotalItems));
}
