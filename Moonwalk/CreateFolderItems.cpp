// VERSION = 2017.03.30
////////////////////////  Создание  списка  видео   ///////////////////////////
#define mpiJsonInfo 40032 // Идентификатор для хранения json информации о фильме
#define mpiKPID     40033 // Идентификатор для хранения ID кинопоиска

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
string    gsUrlBase    = 'http://moonwalk.co'; // База для относительных ссылок
int       gnTotalItems = 0;                    // Счётчик созданных элементов
TDateTime gStart       = Now;                  // Время начала запуска скрипта
string    gsTime       = "02:30:00.000";       // Продолжительность видео

// Регулярные выражения для поиска на странице блоков с информацией о видео
string gsPatternBlock = '(<tr>.*?</tr>)';           // Искомые блоки
string gsCutPage      = '';                         // Обрезка загруженной страницы
string gsPatternTitle = '<tr>\\s*?(<td.*?</td>)';   // Название
string gsPatternLink  = '(http:[^\'"]+/iframe)';    // Ссылка
string gsPatternKP    = 'kinopoisk.ru/film/(.*?)/'; // Код фильма на Kinopoisk
string gsPatternYear  = '<td>(\\d{4})</td>';        // Год
string gsPatternAudio = '';                         // Озвучка / Перевод
string gsPatternPages = 'pagination.*/search_as/(\\d+)[/\\?]'; // Регулярное выражение для поиска максимального номера страницы для дозагрузки
string gsPagesParams  = 'search_as/<PN>';                      // Параметр с номером страницы, который добавляется к ссылке
string gsHeaders = 'Referer: '+gsUrlBase+'/\r\n'+              // HTTP заголовки для запросов
                   'Accept-Encoding: gzip, deflate\r\n'+
                   'Origin: '+gsUrlBase+'\r\n'+
                   'User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/52.0.2743.116 Safari/537.36\r\n';

string    gsTVDBInfo   = "";
bool gbUseSerialKPInfo = false;

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
// Получение информации с Kinopoisk о сериале
void LoadKPSerialInfo() {
  string sID, sData, sHtml, sName, sVal, sHeaders, sYear; int nEpisode, nSeason, n; TRegExpr RE;
  if (gsTVDBInfo!='') return;
  sID = Trim(FolderItem[100500]); // Получаем запомненный kinopoisk ID
  if (sID=='') HmsRegExMatch('/images/(film|film_big)/(\\d+)', mpThumbnail, sID, 2); // Или пытаемся его получить из картинки
  if ((sID!='') && (sID!='0')) {
    // Проверяем, была ли уже загружена информация для такого количества серий
    if ((FolderItem[100508]!='') && (FolderItem[100508]==FolderItem[100509])) {
      gsTVDBInfo = FolderItem[100507];
      return;
    }
    sHeaders = 'Referer: https://kinopoisk.ru/\r\n'+
               'Accept-Encoding: gzip, deflate\r\n'+
               'User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:13.0) Gecko/20100101 Firefox/13.0\r\n';
    sHtml = HmsDownloadURL("https://www.kinopoisk.ru/film/"+sID+"/episodes/", sHeaders, true);
    if (HmsRegExMatch2('<td[^>]+class="news">([^<]+),\\s+(\\d{4})', sHtml, sName, sYear)) {
      // Если получили оригинальное название сериала - пробуем загрузить инфу с картинками
      sName = ReplaceStr(sName, ' ', '_');
      gsTVDBInfo = HmsUtf8Decode(HmsDownloadURL('http://wonky.lostcut.net/tvdb.php?n='+sName+'&y='+sYear, sHeaders, true));
    }
    if (gsTVDBInfo=='') {
      RE = TRegExpr.Create('(<h[^>]+moviename-big.*?)</h', PCRE_SINGLELINE);
      nSeason  = 1;
      nEpisode = 1;
      if (RE.Search(sHtml)) do {
        sName = HmsHtmlToText(re.Match());
        if (HmsRegExMatch("Сезон\\s*?(\\d+)", sName, sVal)) { nSeason=StrToInt(sVal); nEpisode=1; continue; }
        gsTVDBInfo += 's'+Str(nSeason)+'e'+Str(nEpisode)+'=;t='+sName+'|';
        nEpisode++;
      } while (RE.SearchAgain());
    }
    if (gsTVDBInfo=='') gsTVDBInfo = '-';
  }
  FolderItem[100507] = gsTVDBInfo;          // Запоминаем инфу 
  FolderItem[100509] = FolderItem[100508]; // для такого количества серий
}

///////////////////////////////////////////////////////////////////////////////
// Получение реального названия и картинки для серии сериала из gsTVDBInfo
void GetSerialInfo(THmsScriptMediaItem Item, int nSeason, int nEpisode) {
  string sName, sImg;
  if (!gbUseSerialKPInfo ) return; if (gsTVDBInfo=='') LoadKPSerialInfo();
  if (HmsRegExMatch2('s'+Str(nSeason)+'e'+Str(nEpisode)+'=(.*?);t=(.*?)\\|', gsTVDBInfo, sImg, sName)) {
    if (sImg !="") Item[mpiThumbnail] = sImg;
    if (sName!="") Item[mpiTitle    ] = Format("%.2d %s", [nEpisode, sName]);
    Item[mpiSeriesEpisodeNo   ] = nEpisode;
    Item[mpiSeriesSeasonNo    ] = nSeason;
    Item[mpiSeriesEpisodeTitle] = sName;
    Item[mpiSeriesTitle       ] = FolderItem[mpiSeriesTitle];
  }
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки на видео
THmsScriptMediaItem CreateMediaItem(THmsScriptMediaItem Folder, string sTitle, 
string sLink, string sImg, string sTime) {
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, Folder.ItemID);
  Item[mpiTitle     ] = sTitle; // Присваиваем наименование
  Item[mpiThumbnail ] = sImg;   // Картинка
  Item[mpiCreateDate] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0));
  Item[mpiTimeLength] = sTime;  // Продолжительность
  gnTotalItems++;               // Увеличиваем счетчик созданных элементов
  return Item;                  // Возвращаем созданный объект
}

///////////////////////////////////////////////////////////////////////////////
// Создание папки или подкаста
THmsScriptMediaItem CreateFolder(THmsScriptMediaItem ParentFolder, string sName, string sLink, string sImg='') {
  THmsScriptMediaItem Item = ParentFolder.AddFolder(sLink); // Создаём папку с указанной ссылкой
  Item[mpiTitle     ] = sName; // Присваиваем наименование
  Item[mpiThumbnail ] = sImg;  // Картинка
  Item[mpiCreateDate] = DateTimeToStr(IncTime(gStart, 0, -gnTotalItems, 0, 0)); // Для обратной сортировки по дате создания
  
  gnTotalItems++;             // Увеличиваем счетчик созданных элементов
  return Item;                // Возвращаем созданный объект
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
// Загрузка страниц и парсинг
void LoadAndParse() {
  string sHtml, sData, sName, sLink, sImg, sYear, sPage, sVal, sKPID, sTran, sGroupMode="", sGrp="";
  THmsScriptMediaItem Item, Folder = FolderItem; TRegExpr RegEx; bool bGroup, bJustLinks, bYearInTitle=false;
  int i, n, nPages=0, iCnt=0, nGrp=0, nMaxPages=10, nMaxInGroup=100, nMinInGroup=100;
  
  // Проверка установленных дополнительных параметров
  if (HmsRegExMatch('--maxingroup=(\\d+)', mpPodcastParameters, sVal)) nMaxInGroup = StrToInt(sVal);
  if (HmsRegExMatch('--maxpages=(\\d+)'  , mpPodcastParameters, sVal)) nMaxPages   = StrToInt(sVal);
  HmsRegExMatch('--group=(\\w+)', mpPodcastParameters, sGroupMode);
  bYearInTitle = (Pos('--yearintitle', mpPodcastParameters)>0);
  bJustLinks    = (Pos('--justlinks'  , mpPodcastParameters)>0);
  
  //mpFilePath = ReplaceStr(mpFilePath, 'moonwalk/search?', 'moonwalk/search_as?');
  
  if (LeftCopy(mpFilePath, 4) != "http") {
    // Если нет ссылки - делаем поиск названия
    sLink = gsUrlBase+'/moonwalk/search_as?search_for=&commit=%D0%9D%D0%B0%D0%B9%D1%82%D0%B8&sq='+HmsPercentEncode(HmsUtf8Encode(mpTitle));
    sHtml = HmsDownloadURL(sLink, gsHeaders, true);
    nMaxPages = 1;
    
  } else if (RightCopy(mpFilePath, 4)=='.txt') {
    gsPatternBlock = '(.*?)<br>';         // Искомые блоки
    gsPatternTitle = '^(.*?);';           // Название
    gsPatternLink  = 'src="(.*?)"';       // Ссылка
    gsPatternKP    = '^.*?;.*?;(.*?);';   // Код фильма на Kinopoisk
    gsPatternYear  = '^.*?;(\\d{4})';     // Год
    gsPatternAudio = 'iframe&gt;;(.*?);'; // Озвучка / Перевод
    sHtml = HmsDownloadURL(mpFilePath, gsHeaders, true);
    if (HmsRegExMatch('(<link.*?>)', sHtml, sVal)) sHtml = ReplaceStr(sHtml, sVal, '');
    if (HmsRegExMatch('(<meta.*?>)', sHtml, sVal)) sHtml = ReplaceStr(sHtml, sVal, '');
    
  } else {
    sHtml = HmsDownloadURL(mpFilePath, gsHeaders, true);

  }
  sHtml = HmsUtf8Decode(sHtml);       // Декодируем страницу из UTF-8
  sHtml = HmsRemoveLineBreaks(sHtml); // Удаляем переносы строк, для облегчения работы с регулярными выражениями

  // Если указан шаблон поиска максимального номера страницы - применяем
  if ((gsPatternPages!='') && HmsRegExMatch(gsPatternPages, sHtml, sVal)) nPages = StrToIntDef(sVal, 0);

  // Вырезаем только нужный участок текста HTML, где будем искать блоки.
  // Вместо <fromCut> и <toCut> вставляем начало и конец участка HTML, между которыми
  // будем искать блоки текста с сылкой, наименованием и проч.
  if (gsCutPage!='') HmsRegExMatch(gsCutPage, sHtml, sHtml); // ищем в sHtml, результат кладём обратно в sHtml

  // =========================================================================
  // Дозагрузка страниц
  if ((nMaxPages!=0) && (nPages>nMaxPages)) nPages = nMaxPages;
  for (i=2; i<=nPages; i++) {
    HmsSetProgress(Trunc(i*100/nPages));
    HmsShowProgress(Format('%s: Загрузка страницы %d из %d', [mpTitle, i, nPages]));
    sLink = ReplaceStr(mpFilePath, '/search_as?', '/'+ReplaceStr(gsPagesParams, '<PN>', IntToStr(i))+'?');
    sPage = HmsDownloadURL(sLink, 'Referer: '+gsUrlBase, true);
    sPage = HmsUtf8Decode(sPage);
    if (gsCutPage!='') HmsRegExMatch(gsCutPage, sPage, sPage);
    sHtml += sPage;
    if (HmsCancelPressed) break;
  }
  HmsHideProgress();
  // =========================================================================

  // Создаём объект для поиска блоков текста по регулярному выражению,
  // в которых есть информация: ссылка, наименование, ссылка на картинку и проч.
  RegEx = TRegExpr.Create(gsPatternBlock, PCRE_SINGLELINE || PCRE_MULTILINE);
  try {
    // Определяем, если блоков в загруженном более чем gnMaxInGroup, включаем группировку
    if ((sGroupMode!='alph') && (sGroupMode!='year')) {
      i=0; if (RegEx.Search(sHtml)) do i++; while (RegEx.SearchAgain());
      bGroup = (i > nMaxInGroup);
    }
    // Главный цикл поиска блоков
    if (RegEx.Search(sHtml)) do {
      sLink=''; sImg=''; sYear=''; sName=''; sKPID=''; sTran='';
      HmsRegExMatch(gsPatternTitle, RegEx.Match, sName);
      HmsRegExMatch(gsPatternLink , RegEx.Match, sLink);
      HmsRegExMatch(gsPatternKP   , RegEx.Match, sKPID);
      HmsRegExMatch(gsPatternYear , RegEx.Match, sYear);
      HmsRegExMatch(gsPatternAudio, RegEx.Match, sTran);
      if (Trim(sLink)=="") continue;
      
      sName = ReplaceStr(HmsHtmlToText(sName), "/", "-");
      sTran = ReplaceStr(HmsHtmlToText(sTran), "/", "-");
      sLink = HmsExpandLink(sLink, gsUrlBase);
      if (sTran=='Не указан') sTran = '';

      if ((sKPID!='') && (sKPID!='0')) sImg = 'http://www.kinopoisk.ru/images/film/'+sKPID+'.jpg';

      if (sTran!='') sName += ' ['+sTran+']';
      if (bYearInTitle && (sYear!='') && (Pos(sYear, sName)<1)) sName += ' ('+sYear+')';
      
      // Контроль группировки (создаём папку с именем группы)
      if (sGroupMode=='alph') {
        Folder = FolderItem.AddFolder(GetGroupName(sName));
        Folder[mpiFolderSortOrder] = "mpTitle";
      } else if (sGroupMode=='year') {
        Folder = FolderItem.AddFolder(sYear);
        Folder[mpiFolderSortOrder] = "mpTitle";
        Folder[mpiYear           ] = sYear;
      } else if (bGroup) {
        iCnt++; if (iCnt>=nMaxInGroup) { nGrp++; iCnt=0; }
        Folder = FolderItem.AddFolder(Format('%.2d', [nGrp]));
      }

      if (bJustLinks && (Pos('/serial/', sLink) < 1))
        CreateMediaItem(Folder, sName, sLink, sImg, ''); // Создание ссылки на видео
      else {
        Item = CreateFolder(Folder, sName, sLink, sImg);        // Создание папки с фильмом
        Item[100500] = sKPID;
      }

    } while (RegEx.SearchAgain);

  } finally { RegEx.Free(); }

  // Сортируем в базе данных программы созданные элементы
  if (sGroupMode=='alph') {
    FolderItem.Sort('mpTitle');
    for (i=0; i<FolderItem.ChildCount; i++) FolderItem.ChildItems[i].Sort('mpTitle');
  } else if (sGroupMode=='year') FolderItem.Sort('-mpYear');

}

///////////////////////////////////////////////////////////////////////////////
// Создание списка серий сериала с Moonwalk.cc
void CreateMoonwallkLinks(string sLink) {
  String sHtml, sData, sSerie, sVal, sHeaders, sEpisodeTitle, sShowTitle, sID;
  int n, nEpisode, nSeason; TRegExpr RE; THmsScriptMediaItem Item, Folder = FolderItem;
  
  sHeaders = sLink+'/\r\n'+
             'Accept-Encoding: gzip, deflate\r\n'+
             'User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:13.0) Gecko/20100101 Firefox/13.0\r\n'+
             'X-Requested-With: XMLHttpRequest\r\n';
  
  gbUseSerialKPInfo = Pos("--usekpinfo", mpPodcastParameters) > 0;
  
  sHtml = HmsDownloadURL(sLink, 'Referer: '+sHeaders, true);
  if (HmsRegExMatch('<body>\\s*?</body>', sHtml, '')) {
    sLink = ReplaceStr(sLink, 'moonwalk.pw', 'moonwalk.cc');
    sLink = ReplaceStr(sLink, 'moonwalk.co', 'moonwalk.cc');
    sHtml = HmsDownloadURL(sLink, 'Referer: '+sHeaders, true);
  }
  if (HmsRegExMatch('<iframe[^>]+src="(http.*?)"', sHtml, sLink)) sHtml = HmsDownloadURL(sLink, 'Referer: '+sHeaders, true);
  sHtml = HmsRemoveLineBreaks(HmsUtf8Decode(sHtml));
  
  if (Trim(mpSeriesTitle)=='') { FolderItem[mpiSeriesTitle] = mpTitle; HmsRegExMatch('^(.*?)[\\(\\[]', mpTitle, FolderItem[mpiSeriesTitle]); }
  
  // Подсчитываем количество серий и запоминаем в укромном месте
  RE = TRegExpr.Create('(<option[^>]+value="(.*?)".*?</option>)', PCRE_SINGLELINE);
  n = 0; if (RE.Search(sHtml)) do n++; while (RE.SearchAgain()); FolderItem[100508] = Str(n);
  
  if (HmsRegExMatch('<select[^>]+id="episode"(.*?)</select>', sHtml, '')) {
    if (HmsRegExMatch('season=(\\d+)', sLink, sVal)) {
      gsTime  = '00:45:00.000';
      nSeason = StrToInt(sVal);
      HmsRegExMatch('<select[^>]+id="episode"(.*?)</select>', sHtml, sData);
      RE = TRegExpr.Create('(<option[^>]+value="(.*?)".*?</option>)', PCRE_SINGLELINE);
      if (RE.Search(sData)) do {
        sSerie = ReplaceStr(HmsHtmlToText(RE.Match(1)), '/', '-');
        // Форматируем номер в два знака
        if (HmsRegExMatch("(\\d+)", sSerie, sVal)) {
          nEpisode = StrToInt(sVal);
          sSerie   = Format("%.2d %s", [nEpisode, ReplaceStr(sSerie, sVal, "")]);
        }
        Item = CreateMediaItem(Folder, sSerie, sLink+'&episode='+RE.Match(2), mpThumbnail, gsTime);
        GetSerialInfo(Item, nSeason, nEpisode);
      } while (RE.SearchAgain());
      RE.Free();
    } else if (HmsRegExMatch('<select[^>]+id="season"(.*?)</select>', sHtml, sData)) {
      RE = TRegExpr.Create('(<option[^>]+value="(.*?)".*?</option>)', PCRE_SINGLELINE);
      // Подсчитываем количество сезонов
      n = 0; if (RE.Search(sData)) do { n++; HmsRegExMatch("(\\d+)", HmsHtmlToText(RE.Match(1)), sVal); } while (RE.SearchAgain());
      if ( (n == 1) && (sVal == '1') ) {
        // Если сезон всего один и номер его = 1, то сразу выкатываем серии
        gsTime   = '00:45:00.000';
        nSeason  = 1;
        HmsRegExMatch('<select[^>]+id="episode"(.*?)</select>', sHtml, sData);
        if (RE.Search(sData)) do {
          sSerie = ReplaceStr(HmsHtmlToText(RE.Match(1)), '/', '-');
          // Форматируем номер в два знака
          if (HmsRegExMatch("(\\d+)", sSerie, sVal)) {
            nEpisode = StrToInt(sVal);
            sSerie   = Format("%.2d %s", [nEpisode, ReplaceStr(sSerie, sVal, "")]);
          }
          Item = CreateMediaItem(Folder, sSerie, sLink+'?season=1&episode='+RE.Match(2), mpThumbnail, gsTime);
          GetSerialInfo(Item, nSeason, nEpisode);
        } while (RE.SearchAgain());
      } else {
        if (RE.Search(sData)) do {
          sSerie = ReplaceStr(HmsHtmlToText(RE.Match(1)), '/', '-');
          Item = CreateFolder(Folder, sSerie, sLink+'?season='+RE.Match(2), mpThumbnail);
          Item[mpiSeriesTitle] = FolderItem[mpiSeriesTitle];
        } while (RE.SearchAgain());
      }
      RE.Free();
    }
  } else {
    CreateMediaItem(Folder, mpTitle, sLink, mpThumbnail, gsTime);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Проверка и обновление скриптов подкаста
void CheckPodcastUpdate() {
  THmsScriptMediaItem Podcast=FolderItem; TJsonObject JSON, JFILE; TJsonArray JARRAY;
  string sData, sName, sLang, sExt, sMsg; int i, mpiTimestamp=100602, mpiSHA, mpiScript; bool bChanges=false;
  
  if ((Trim(Podcast[550])=='') && (Podcast.ItemParent!=nil)) Podcast = Podcast.ItemParent; // Ищем скрипт получения ссылки на поток
  // Если после последней проверки прошло меньше получаса - валим
  if ((Podcast.ItemParent==nil) || (DateTimeToTimeStamp1970(Now, false)-StrToIntDef(Podcast[mpiTimestamp], 0) < 1800)) return;
  Podcast[mpiTimestamp] = DateTimeToTimeStamp1970(Now, false); // Запоминаем время проверки
  sData = HmsDownloadURL('https://api.github.com/repos/WendyH/HMS-podcasts/contents/Moonwalk', "Accept-Encoding: gzip, deflate", true);
  JSON  = TJsonObject.Create();
  try {
    JSON.LoadFromString(sData);
    JARRAY = JSON.AsArray(); if (JARRAY==nil) return;
    for (i=0; i<JARRAY.Length; i++) {        // Обходим в цикле все файлы в папке github
      JFILE = JARRAY[i]; if(JFILE.S['type']!='file') continue;
      sName = ChangeFileExt(JFILE.S['name'], ''); sExt = ExtractFileExt(JFILE.S['name']);
      switch (sExt) { case'.cpp':sLang='C++Script'; case'.pas':sLang='PascalScript'; case'.vb':sLang='BasicScript'; case'.js':sLang='JScript'; default:sLang=''; } // Определяем язык по расширению файла
      if      (sName=='CreatePodcastFeeds'   ) { mpiSHA=100701; mpiScript=571; sMsg='Требуется запуск "Создать ленты подкастов"'; } // Это сприпт создания покаст-лент   (Alt+1)
      else if (sName=='CreateFolderItems'    ) { mpiSHA=100702; mpiScript=530; sMsg='Требуется обновить раздел заново';           } // Это скрипт чтения списка ресурсов (Alt+2)
      else if (sName=='PodcastItemProperties') { mpiSHA=100703; mpiScript=510; sMsg='Требуется обновить раздел заново';           } // Это скрипт чтения дополнительных в RSS (Alt+3)
      else if (sName=='MediaResourceLink'    ) { mpiSHA=100704; mpiScript=550; sMsg=''; }                                           // Это скрипт получения ссылки на ресурс  (Alt+4)
      else continue;                         // Если не известный файл - пропускаем
      if (Podcast[mpiSHA]!=JFILE.S['sha']) { // Проверяем, требуется ли обновлять скрипт?
        sData = HmsDownloadURL(JFILE.S['download_url'], "Accept-Encoding: gzip, deflate", true); // Загружаем скрипт
        if (sData=='') continue;                                                     // Если не получилось загрузить, пропускаем
        Podcast[mpiScript+0] = HmsUtf8Decode(ReplaceStr(sData, '\xEF\xBB\xBF', '')); // Скрипт из unicode и убираем BOM
        Podcast[mpiScript+1] = sLang;                                                // Язык скрипта
        Podcast[mpiSHA     ] = JFILE.S['sha']; bChanges = true;                      // Запоминаем значение SHA скрипта
        HmsLogMessage(1, Podcast[mpiTitle]+": Обновлён скрипт подкаста "+sName);     // Сообщаем об обновлении в журнал
        if (sMsg!='') FolderItem.AddFolder(' !'+sMsg+'!');                           // Выводим сообщение как папку
      }
    } 
  } finally { JSON.Free; if (bChanges) HmsDatabaseAutoSave(true); }
} //Вызов в главной процедуре: if ((Pos('--nocheckupdates' , mpPodcastParameters)<1) && (mpComment=='--update')) CheckPodcastUpdate();

///////////////////////////////////////////////////////////////////////////////
// Проверка актуальности версии функции GetLink_Moonwalk в скриптах
void CheckMoonwalkFunction() {
  string sData, sFuncOld, sFuncNew; THmsScriptMediaItem Podcast=FolderItem; 
  int nTimePrev, nTimeNow, mpiTimestamp=100602, mpiMWVersion=100603; TJsonObject JSON;
  string sPatternMoonwalkFunction = "(// Получение ссылки с moonwalk.cc.*?// Конец функции поулчения ссылки с moonwalk.cc)";

  if ((Trim(Podcast[550])=='') && (Podcast.ItemParent!=nil)) Podcast = Podcast.ItemParent; // Ищем скрипт получения ссылки на поток
  // Если после последней проверки прошло меньше получаса - валим
  if ((Podcast.ItemParent==nil) || (DateTimeToTimeStamp1970(Now, false)-StrToIntDef(Podcast[mpiTimestamp], 0) < 1800)) return;
  Podcast[mpiTimestamp] = DateTimeToTimeStamp1970(Now, false); // Запоминаем время проверки
  if (HmsRegExMatch(sPatternMoonwalkFunction, Podcast[550], sFuncOld, 1, PCRE_SINGLELINE)) {
    sData = HmsUtf8Decode(HmsDownloadURL("https://api.github.com/gists/3092dc755ad4a6c412e2fcd17c28d096", "Accept-Encoding: gzip, deflate", true));
    JSON  = TJsonObject.Create();
    try {
      JSON.LoadFromString(sData);
      sFuncNew = JSON.S['files\\GetLink_Moonwalk.cpp\\content'];
      HmsRegExMatch(sPatternMoonwalkFunction, sFuncNew, sFuncNew, 1, PCRE_SINGLELINE);
      if ((sFuncNew!='') && (Podcast[mpiMWVersion]!=JSON.S['updated_at'])) {
        Podcast[550] = ReplaceStr(Podcast[550], sFuncOld, sFuncNew); // Заменяем старую функцию на новую
        Podcast[mpiMWVersion] = JSON.S['updated_at'];
        HmsLogMessage(2, Podcast[mpiTitle]+": Обновлена функция получения ссылки с moonwalk.cc!");
      }
    } finally { JSON.Free; }
  }
  //Вызов в главной процедуре: if (Pos('--noupdatemoonwalk', mpPodcastParameters)<1) CheckMoonwalkFunction();
}

///////////////////////////////////////////////////////////////////////////////
//                    Г Л А В Н А Я    П Р О Ц Е Д У Р А                     //
///////////////////////////////////////////////////////////////////////////////
{
  FolderItem.DeleteChildItems(); // Удаляем созданные ранее элементы в текущей папке

  if ((Pos('--nocheckupdates' , mpPodcastParameters)<1) && (mpComment=='--update')) CheckPodcastUpdate(); // Проверка обновлений подкаста
  if (Pos('--noupdatemoonwalk', mpPodcastParameters)<1) CheckMoonwalkFunction();
  
  if (HmsRegExMatch("/(serial|movie|video)/", mpFilePath, '')) {
    CreateMoonwallkLinks(mpFilePath); // Cоздание ссылок на фильм или сериал
  } else {
    LoadAndParse();                   // Запускаем загрузку страниц и создание папок видео
  }

  HmsLogMessage(1, Podcast[mpiTitle]+' "'+mpTitle+'": Создано элементов - '+IntToStr(gnTotalItems));
}

