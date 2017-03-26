// VERSION = 2017.03.26
////////////////////////  Создание  списка  видео   ///////////////////////////

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
string    gsUrlBase     = "http://allcc.ru";
TDateTime gStart        = Now;
int       gnTotalItems  = 0;
int       gnDefaultTime = 4000;

// Регулярные выражения для поиска на странице блоков с информацией о видео
string gsCutPage      = '';
string gsPatternBlock = 'class="news"(.*?)comm_link';
string gsPatternTitle = 'alt=[\'"](.*?)[\'"]';
string gsPatternLink  = '<a[^>]+href=[\'"](.*?)[\'"]';
string gsPatternYear  = '<span>\\d+[^<]+(\\d{4})';
string gsPatternImg   = '<img[^>]+src=[\'"](.*?)[\'"]';
string gsPatternSkip  = '<img[^>]+src=[\'"](.*?)[\'"]';
string gsPatternPages = '.*/page/\\d+/">(\\d+)';      // Поиск максимального номера страницы
string gsPagesParam   = '/page/<PN>/';

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
// Получение название группы из имени
string GetGroupName(string sName) {
  string sGrp = '#';
  if (HmsRegExMatch('([A-ZА-Я0-9])', sName, sGrp, 1, PCRE_CASELESS)) sGrp = Uppercase(sGrp);
  if (HmsRegExMatch('[0-9]', sGrp, sGrp)) sGrp = '#';
  if (HmsRegExMatch('[A-Z]', sGrp, sGrp)) sGrp = 'A..Z';
  return sGrp;
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки на видео
THmsScriptMediaItem CreateMediaItem(THmsScriptMediaItem Folder, string sTitle, string sLink, string sImg, int nTime) {
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, Folder.ItemID);
  Item[mpiTitle     ] = sTitle;
  Item[mpiThumbnail ] = sImg;
  Item[mpiCreateDate] = IncTime(gStart,0,-gnTotalItems,0,0); gnTotalItems++;
  Item[mpiTimeLength] = HmsTimeFormat(nTime)+'.000';
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки-ошибки
void CreateErrorItem(string sMsg) {
  THmsScriptMediaItem Item = HmsCreateMediaItem('Err', FolderItem.ItemID);
  Item[mpiTitle     ] = sMsg;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/icons/symbol-error.png';
}

///////////////////////////////////////////////////////////////////////////////
// Создание папки или подкаста
THmsScriptMediaItem CreateFolder(THmsScriptMediaItem Parent, string sName, string sLink, string sImg=mpThumbnail) {
  THmsScriptMediaItem Item = Parent.AddFolder(sLink); // Создаём папку с указанной ссылкой
  Item[mpiTitle      ] = sName;         // Присваиваем наименование
  Item[mpiThumbnail  ] = sImg;          // Картинка папки
  Item[mpiCreateDate ] = IncTime(gStart, 0, -gnTotalItems, 0, 0); gnTotalItems++;
  Item[mpiSeriesTitle] = mpSeriesTitle; // Наследуем название сериала (если есть)
  Item[mpiYear       ] = mpYear;        // Наследуем год сериала (если есть)
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Загрузка страниц и парсинг
void LoadAndParse() {
  string sHtml, sName, sLink, sImg, sYear, sPage, sVal, sGroupKey="", sGrp="";
  THmsScriptMediaItem Item, Folder = FolderItem; TRegExpr RegEx;
  int i, n, nPages=0, iCnt=0, nGrp=0, nMaxPages=10, nMaxInGroup=100, nMinInGroup=30;
  
  // Проверка установленных дополнительных параметров
  if (HmsRegExMatch('--miningroup=(\\d+)', mpPodcastParameters, sVal)) nMinInGroup = StrToInt(sVal);
  if (HmsRegExMatch('--maxingroup=(\\d+)', mpPodcastParameters, sVal)) nMaxInGroup = StrToInt(sVal);
  if (HmsRegExMatch('--maxpages=(\\d+)'  , mpPodcastParameters, sVal)) nMaxPages   = StrToInt(sVal);
  HmsRegExMatch('--group=(\\w+)'         , mpPodcastParameters, sGroupKey);
  bool bYearInTitle = Pos('--yearintitle', mpPodcastParameters) > 0;
  
  sHtml = HmsDownloadURL(mpFilePath, 'Accept-Encoding: gzip, deflate', true);
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
    sLink = mpFilePath + ReplaceStr(gsPagesParam, '<PN>', IntToStr(i));
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
    i=0; if (RegEx.Search(sHtml)) do i++; while (RegEx.SearchAgain);
    if ((sGroupKey=="") && (i > nMaxInGroup)) sGroupKey="quant";
    else if (i < nMinInGroup) sGroupKey=""; // Не группируем, если их мало
    // Главный цикл поиска блоков
    if (RegEx.Search(sHtml)) do {
      sLink=''; sName=''; sImg=''; sYear='';
      HmsRegExMatch(gsPatternTitle, RegEx.Match, sName);
      HmsRegExMatch(gsPatternLink , RegEx.Match, sLink);
      HmsRegExMatch(gsPatternImg  , RegEx.Match, sImg );
      HmsRegExMatch(gsPatternYear , RegEx.Match, sYear);
      if (Trim(sLink)=="") continue;
      // На сайте allcc.ru видео только с такой картинкой, остальные - просто новости
      if (Pos('no_image.jpg', RegEx.Match) < 1) continue; 
      
      sName = HmsHtmlToText(sName);
      sLink = HmsExpandLink(sLink, gsUrlBase);
      sImg  = HmsExpandLink(sImg , gsUrlBase);
      sName = Trim(ReplaceStr(sName, 'онлайн', ''));
      
      // Добавляем год в название, если установлен флаг и этого года в названии нет
      if (bYearInTitle && (sYear!='') && (Pos(sYear, sName)<1)) sName += ' ('+sYear+')';
      
      // Контроль группировки (создаём папку с именем группы)
      switch (sGroupKey) {
        case "quant": { sGrp = Format("%.2d", [nGrp]); iCnt++; if (iCnt>=nMaxInGroup) { nGrp++; iCnt=0; } }
        case "alph" : { sGrp = GetGroupName(sName); }
        case "year" : { sGrp = sYear; }
        default     : { sGrp = "";    }
      }
      if (Trim(sGrp)!="") Folder = FolderItem.AddFolder(sGrp);
      
      CreateMediaItem(Folder, sName, sLink, sImg, gnDefaultTime); // Создание ссылки на видео
      
    } while (RegEx.SearchAgain);
  } finally { RegEx.Free(); }
  // Сортируем в базе данных программы созданные элементы
  if      (sGroupKey=="alph") FolderItem.Sort("mpTitle" );
  else if (sGroupKey=="year") FolderItem.Sort("-mpTitle");

  HmsLogMessage(1, Podcast[mpiTitle]+' "'+mpTitle+'": Создано элементов - '+IntToStr(gnTotalItems));
}

///////////////////////////////////////////////////////////////////////////////
// Проверка и обновление скриптов подкаста
void CheckPodcastUpdate() {
  string sData, sName, sLang, sExt, sMsg; int i, mpiTimestamp=100602, mpiSHA, mpiScript;
  TJsonObject JSON, JFILE; TJsonArray JARRAY; bool bChanges=false;
  
  // Если после последней проверки прошло меньше получаса - валим
  if ((Trim(Podcast[550])=='') || (DateTimeToTimeStamp1970(Now, false)-StrToIntDef(Podcast[mpiTimestamp], 0) < 1800)) return;
  Podcast[mpiTimestamp] = DateTimeToTimeStamp1970(Now, false); // Запоминаем время проверки
  sData = HmsDownloadURL('https://api.github.com/repos/WendyH/HMS-podcasts/contents/AllComedy', "Accept-Encoding: gzip, deflate", true);
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
      else continue;                         // Если файл не определён - пропускаем
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
} //Вызов в главной процедуре: if (Pos('--nocheckupdates', mpPodcastParameters) < 1) CheckPodcastUpdate();

///////////////////////////////////////////////////////////////////////////////
//                      Г Л А В Н А Я   П Р О Ц Е Д У Р А                    //
// ----------------------------------------------------------------------------
{
  if (InteractiveMode && (HmsCurrentMediaTreeItem.ItemClassName=='TVideoPodcastsFolderItem')) {
    if (gsUserVariable1== '-nomsg-') return;
    HmsLogMessage(1, "Завязывайте таким образом обновлять подкасты! Вы делаете кучу ненужных запросов на сайты.");
    HmsLogMessage(2, "Обновлять подкаст можно только в конкретной категории.");
    ShowMessage("Таким образом подкаст обновлять запрещено!\nОбновите конкретную категорию.");
    gsUserVariable1 = '-nomsg-';
    return;
  }
  gsUserVariable1 = '';

  FolderItem.DeleteChildItems(); // Удаление существующих ссылок

  if ((Pos('--nocheckupdates', mpPodcastParameters)<1) && (mpComment=='--update')) CheckPodcastUpdate(); // Проверка обновлений подкаста

  LoadAndParse();     // Загрузка страниц, поиск и создание ссылок

}