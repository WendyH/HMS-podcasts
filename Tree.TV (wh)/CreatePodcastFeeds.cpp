///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
string gsUrlBase="http://tree.tv"; int gnItemsAdded=0; TDateTime gTimeStart=Now;

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

// -------------------------------------- Функция возвращает скрипт поиска ----
string GetSearchDynamicScript() {
  string sScript='', sLink, sHtml, sRE;

  // Загружаем с форума тему с сообщением, где выложен актуальный скрипт  
  sLink = 'http://homemediaserver.ru/forum/viewtopic.php?f=15&t=2793&p=17395#p17395';
  sHtml = HmsUtf8Decode(HmsDownloadURL(sLink, 'Referer: '+sLink, true));
  
  // Задаём регулярное выражения поиска скрипта (текст между ключевыми словами) 
  sRE   = 'BeginDynamicSearchScript\\*/(.*?)/\\*EndDynamicSearchScript';
  
  // Вырезаем из сообщения текст скрипта
  HmsRegExMatch(sRE, sHtml, sScript, 1, PCRE_SINGLELINE);
  sScript = HmsHtmlToText(sScript, 1251);   // Конвертируем в нужную кодировку
  sScript = ReplaceStr(sScript, #160, ' '); // Удаляем символ 'no break space'

  return sScript;
}
                                                                                                     
// -------------- Функция создания динамической папки с указанным скриптом ----                      
THmsScriptMediaItem CreateDynamicItem(THmsScriptMediaItem prntItem, char sTitle,                     
                                      char sLink, char &sScript='') {                                
  THmsScriptMediaItem Folder;

  Folder = prntItem.AddFolder(sLink, false, 32);
  Folder[mpiTitle]      = sTitle;
  Folder[mpiCreateDate] = VarToStr(IncTime(Now,0,-prntItem.ChildCount,0,0));
  Folder[200] = 5;           // mpiFolderType
  Folder[500] = sScript;     // mpiDynamicScript
  Folder[501] = 'C++Script'; // mpiDynamicSyntaxType
  Folder[mpiFolderSortOrder] = -mpiCreateDate;
  return Folder;
}

// ------------------------- Замена в тексте значения текстовой переменной ----
void ReplaceVarValue(string &sText, string sVarName, string sNewVal) {
  string sVal, sVal2;

  // Замена в тексте скрипта значение указанной переменной sVarName на значение sNewVal
  if (HmsRegExMatch2("("+sVarName+"\\s*?=.*?';)", sText, sVal, sVal2)) {
     HmsRegExMatch(sVarName+"\\s*?=\\s*?'(.*)'", sVal, sVal2); 
     sText = ReplaceStr(sText, sVal, ReplaceStr(sVal , sVal2, sNewVal));
  }
}

// -------------------------------------------------- Создание папки ПОИСК ----
void CreateSearchFolder(THmsScriptMediaItem prntItem) {
  THmsScriptMediaItem Folder; string sScript, sVal;
  
  sScript = GetSearchDynamicScript(); // Получаем скрипт поиска с форума HMS

  // И меняем значения переменных на свои
  ReplaceVarValue(sScript, 'gsSuggestQuery'  , 'http://tree.tv/search/index/autocomplete/?term=');
  ReplaceVarValue(sScript, 'gsSuggestRegExpr', '"my_name":\\s*?"(.*?)",');
  
  // Создаём динамическую папку для набора текста с нашим загруженным скриптом
  CreateDynamicItem(prntItem, '"Набрать текст"', '-SearchCommands', sScript);
}

// ------------------------ Создание элемента структуры (папка или подкаст) ---
THmsScriptMediaItem CreateItem(THmsScriptMediaItem Folder, string sLink, string sTitle="", bool bForceFolder=false) {
  THmsScriptMediaItem Item;

  // Если название не указано, а только ссылка - то это папка
  if (Trim(sTitle)=="") {sTitle = sLink; bForceFolder=true;}
  
  if (sLink[1]=='/') sLink = HmsExpandLink(sLink, gsUrlBase); // Дополняем относительную ссылку
  Item  = Folder.AddFolder(sLink, bForceFolder); // Создаём подкаст/папку
  Item[mpiTitle          ] = sTitle;
  Item[mpiCreateDate     ] = VarToStr(IncTime(gTimeStart,0,-gnItemsAdded,0,0));
  Item[mpiFolderSortOrder] = "-mpCreateDate";
  gnItemsAdded++;
  return Item;
}

// ------------------------------------------- Создание структуры подкаста ----
void CreateStructure() {
  THmsScriptMediaItem Folder, Item; int i, nYear, nMonth, nDay; string sLink;

  // Создание папки "Поиск"
  Folder = CreateItem(FolderItem, '-SearchFolder', '00 Поиск', true);
  Folder[mpiFolderSortOrder] = "mpTitle";
  CreateSearchFolder(Folder);

  // Создание подкастов и папок
  Folder = CreateItem(FolderItem, '/films'    , '01 Новинки'    );
  Folder[mpiPodcastParameters] = '--maxpages=3';

  Folder = CreateItem(FolderItem, '/serials'  , '02 Сериалы'    );
  Folder[mpiFolderSortOrder  ] = "mpTitle";
  Folder[mpiPodcastParameters] = '--maxpages=160 --group=alph';

  Folder = CreateItem(FolderItem, '/multfilms', '03 Мультфильмы');
  Folder[mpiFolderSortOrder  ] = "-mpTitle";
  Folder[mpiPodcastParameters] = '--maxpages=110 --group=year';

  Folder = CreateItem(FolderItem, '/onlinetv' , '04 ТВ-передачи');
  Folder[mpiFolderSortOrder  ] = "-mpTitle";
  Folder[mpiPodcastParameters] = '--maxpages=110 --group=year';

  Folder = CreateItem(FolderItem, '/anime'    , '05 Аниме');
  Folder[mpiFolderSortOrder  ] = "-mpTitle";
  Folder[mpiPodcastParameters] = '--maxpages=110 --group=alph';

  Folder = CreateItem(FolderItem, '06 По жанрам');
  Folder[mpiPodcastParameters] = '--maxpages=20 --group=quant';

  sLink  = '/films/sortType/new/janrs/';
  CreateItem(Folder, sLink+'570'  , 'Аниме'    );
  CreateItem(Folder, sLink+'552'  , 'Арт-Хаус' );
  CreateItem(Folder, sLink+'569'  , 'Биография');
  CreateItem(Folder, sLink+'567'  , 'Боевик'   );
  CreateItem(Folder, sLink+'568'  , 'Вестерн'  );
  CreateItem(Folder, sLink+'566'  , 'Военный'  );
  CreateItem(Folder, sLink+'565'  , 'Детектив' );
  CreateItem(Folder, sLink+'564'  , 'Детский'  );
  CreateItem(Folder, sLink+'8422' , 'Документальный');
  CreateItem(Folder, sLink+'561'  , 'Драма'         );
  CreateItem(Folder, sLink+'560'  , 'Исторический'  );
  CreateItem(Folder, sLink+'562'  , 'Катастрофа' );
  CreateItem(Folder, sLink+'559'  , 'Комедия'    );
  CreateItem(Folder, sLink+'36274', 'Короткометражка');
  CreateItem(Folder, sLink+'557'  , 'Криминал'   );
  CreateItem(Folder, sLink+'556'  , 'Мелодрама'  );
  CreateItem(Folder, sLink+'550'  , 'Мистика'    );
  CreateItem(Folder, sLink+'555'  , 'Музыкальный');
  CreateItem(Folder, sLink+'553'  , 'Мюзикл'     );
  CreateItem(Folder, sLink+'551'  , 'Приключения');
  CreateItem(Folder, sLink+'549'  , 'Семейный'   );
  CreateItem(Folder, sLink+'637'  , 'Сказка'     );
  CreateItem(Folder, sLink+'548'  , 'Спорт'      );
  CreateItem(Folder, sLink+'646'  , 'ТВ Передачи');
  CreateItem(Folder, sLink+'547'  , 'Триллер'    );
  CreateItem(Folder, sLink+'546'  , 'Ужасы'      );
  CreateItem(Folder, sLink+'545'  , 'Фантастика' );
  CreateItem(Folder, sLink+'537'  , 'Фэнтези'    );
  CreateItem(Folder, sLink+'563'  , 'Эротика'    );

  Folder = CreateItem(FolderItem, '07 По годам');
  Folder[mpiFolderSortOrder  ] = "-mpTitle";
  Folder[mpiPodcastParameters] = '--maxpages=20 --group=quant';
  DecodeDate(Now, nYear, nMonth, nDay); if (nMonth>10) nYear++; 
  for (i=nYear; i>1990; i--) {
    Item = CreateItem(Folder, '/films/sortType/new/sortYear/'+IntToStr(i), IntToStr(i));
    Item[mpiFolderSortOrder  ] = "mpTitle";
  }

  Folder = CreateItem(FolderItem, '08 По наградам');
  Folder[mpiPodcastParameters] = '--maxpages=20 --group=year';
  sLink  = '/films/sortType/new/awards/'; 
  CreateItem(Folder, sLink+'44058', 'Оскар');
  CreateItem(Folder, sLink+'44557', 'Золотой Глобус');
  CreateItem(Folder, sLink+'44559', 'Сатурн');
  CreateItem(Folder, sLink+'44562', 'Золотой медведь');
  CreateItem(Folder, sLink+'44563', 'Золотой лев');
  CreateItem(Folder, sLink+'44059', 'Золотая пальмовая ветвь');
  CreateItem(Folder, sLink+'44558', 'Ника');
  CreateItem(Folder, sLink+'44565', 'Хрустальный глобус');
  CreateItem(Folder, sLink+'44560', 'Эмми');
  CreateItem(Folder, sLink+'44561', 'Золотая малина');

  Folder = CreateItem(FolderItem, '/films'    , '01 Новинки'    );
  Folder = CreateItem(FolderItem, '/serials'  , '02 Сериалы'    );
  Folder = CreateItem(FolderItem, '/multfilms', '03 Мультфильмы');
  Folder = CreateItem(FolderItem, '/onlinetv' , '04 ТВ-передачи');
  Folder = CreateItem(FolderItem, '/anime'    , '05 Аниме');

  Folder = CreateItem(FolderItem, '09 По рейтингу');
  Folder[mpiPodcastParameters] = '--maxpages=10 --maxingroup=200 --group=none';
  CreateItem(Folder, '/films/sortType/rate'    , 'Фильмы');
  CreateItem(Folder, '/multfilms/sortType/rate', 'Мультфильмы');
  CreateItem(Folder, '/serials/sortType/rate'  , 'Сериалы');
  CreateItem(Folder, '/onlinetv/sortType/rate' , 'ТВ-передачи');
  CreateItem(Folder, '/anime/sortType/rate'    , 'Аниме');

  Folder = CreateItem(FolderItem, '10 По просмотрам');
  Folder[mpiPodcastParameters] = '--maxpages=10 --maxingroup=200 --group=none';
  CreateItem(Folder, '/films/sortType/view'    , 'Фильмы');
  CreateItem(Folder, '/multfilms/sortType/view', 'Мультфильмы');
  CreateItem(Folder, '/serials/sortType/view'  , 'Сериалы');
  CreateItem(Folder, '/onlinetv/sortType/view' , 'ТВ-передачи');
  CreateItem(Folder, '/anime/sortType/view'    , 'Аниме');
}

// ---------------------- Проверка подтверждения на пересоздания структуры ----
bool CheckForSure() {
  int nAnsw; string sMsg;
  
  sMsg = 'ПЕРЕСОЗДАНИЕ СТРУКТУРЫ ПОДКАСТА.\n\n'+
         'Удалить существующую структуру, очистить ссылки???\n\n'+
         '"Да"  - будут удалены все вложенные папки и ссылки. Структура будет создана заного.\n'+
         '"Нет" - будет создана структура поверх существующих ссылок.\n'+
         '"Отмена" - не пересоздавать структуру.\n';

  if (FolderItem.HasChildItems) {
    nAnsw = MessageDlg(sMsg, 0, 3+8, 0);
    if (nAnsw == mrCancel) return false;
    if (nAnsw == mrYes   ) FolderItem.DeleteChildItems();
  }
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////
//                      Г Л А В Н А Я   П Р О Ц Е Д У Р А                    //
// ----------------------------------------------------------------------------
{
  if (!CheckForSure()) return; // Удаление существующих ссылок

  CreateStructure(); // Создание структуры подкаста

  HmsLogMessage(1, mpTitle+': Создано ссылок - '+IntToStr(gnItemsAdded));
}