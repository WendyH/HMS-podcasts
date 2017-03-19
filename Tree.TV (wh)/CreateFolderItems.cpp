// VERSION = 2017.03.19
///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
string gsUrlBase     = "http://tree.tv";
string gsGroupingKey = "none";
TDateTime gTimeStart = Now; bool gbYearInTitle; 
int gnMaxPages=10, gnMaxInGroup=100, gnItemsAdded=0;

// Регулярные выражения для поиска на странице блоков с информацией о видео
string gsPatternBlock = '(<div[^>]+(item_open_content|class="item").*?)<div[^>]+class="clear"></div>\\s*?</div>\\s*?</div>';
string gsPatternTitle = '(<h2.*?</h2>)';                       // Название
string gsPatternLink  = '<a[^>]+href="(/film.*?)"';            // Ссылка
string gsPatternYear  = '(?:>Год<|smoll_year).*?(\\d{4})';
string gsPatternImg   = 'smoll_year.*?<img[^>]+src="(.*?)"';
string gsPatternPages = '.*<a[^>]+rel=(\\d+)[^>]+"page"';      // Поиск максимального номера страницы
string gsPagesParam   = '/page/<PN>';

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

// ------------------------------------ Получение название группы из имени ----
string GetGroupName(string sName) {
  string sGrp = '#';
  if (HmsRegExMatch('([A-ZА-Я0-9])', sName, sGrp, 1, PCRE_CASELESS)) sGrp = Uppercase(sGrp);
  if (HmsRegExMatch('[0-9]', sGrp, sGrp)) sGrp = '#';
  if (HmsRegExMatch('[A-Z]', sGrp, sGrp)) sGrp = 'A..Z';
  return sGrp;
}

// ------------------------------------------------------- Создание ссылки ----
THmsScriptMediaItem CreateFolder(THmsScriptMediaItem ParentFolder, string sLink, string sTitle="", string sYear="", string sImg="") {
  THmsScriptMediaItem Item = ParentFolder.AddFolder(sLink);
  Item[mpiTitle          ] = sTitle;
  Item[mpiCreateDate     ] = VarToStr(IncTime(gTimeStart,0,-gnItemsAdded,0,0));
  Item[mpiFolderSortOrder] = "-mpCreateDate";
  Item[mpiThumbnail      ] = sImg;
  Item[mpiYear           ] = sYear; // Для сортировки по годам
  gnItemsAdded++;
  return Item; 
}

// ------------------------------------------ Проверка параметров подкаста ----
void CheckParameters () {
  string sVal;
  
  gbYearInTitle = (Pos('--yearintitle', mpPodcastParameters) > 0);
  
  HmsRegExMatch('--group=(\\w+)', mpPodcastParameters, gsGroupingKey);
  
  if (HmsRegExMatch('--maxingroup=(\\d+)', mpPodcastParameters, sVal)) gnMaxInGroup = StrToInt(sVal);
  if (HmsRegExMatch('--maxpages=(\\d+)'  , mpPodcastParameters, sVal)) gnMaxPages   = StrToInt(sVal);
}

// -------------------- Загрузка страниц, поиск и создание ссылок на видео ----
void CreateLinks() {
  string sHtml, sData, sPage, sName, sLink, sYear, sImg, sVal; TRegExpr RE;
  int i, n, nPages, iCnt=0, nGrp=0, nSortCount=0; char sGrp="";
  THmsScriptMediaItem Item, Folder = FolderItem; 
  
  sLink = mpFilePath;
  // Если нет ссылки - формируем ссылку для поиска названия
  if (LeftCopy(mpFilePath, 4) != "http") {
    
    if (Length(mpTitle)<4) mpTitle += " :::"; // Фишка обхода ограничения на минимальную длину в 4 символа (двоеточие при самом поиске не учитывается)
      
    sHtml = HmsSendRequestEx('tree.tv', '/search/index', 'POST',
    'application/x-www-form-urlencoded; Charset=UTF-8', 
    gsUrlBase+'/\r\nAccept-Encoding: gzip, deflate', 
    'usersearch='+HmsHttpEncode(HmsUtf8Encode(mpTitle)), 80, 0, '', true);
    gnMaxPages = 0;
    
  } else {
    sHtml = HmsDownloadURL(sLink, 'Referer: '+gsUrlBase, true);
    
  }
  sHtml = HmsUtf8Decode(sHtml);
  
  // Дозагрузка страниц (если задан шаблон поиска максимального номера сраницы)
  if ((gsPatternPages!='') && HmsRegExMatch(gsPatternPages, sHtml, sVal, 1, PCRE_SINGLELINE)) {
    nPages = StrToInt(sVal); // Номер последней страницы
    if (nPages > gnMaxPages) nPages = gnMaxPages;
    for (i=2; i <= nPages; i++) {
      HmsSetProgress(Trunc(i*100/nPages));
      HmsShowProgress(Format('%s: Загрузка страницы %d из %d', [mpTitle, i, nPages]));
      sHtml+= HmsUtf8Decode(HmsDownloadURL(sLink+ReplaceStr(gsPagesParam, '<PN>', IntToStr(i)), 'Referer: '+gsUrlBase, true));
      if (HmsCancelPressed()) break;
    }
    HmsHideProgress();
  }
  
  sHtml = HmsRemoveLineBreaks(sHtml);
  
  // Создание ссылок
  nSortCount = 0;
  RE = TRegExpr.Create(gsPatternBlock, PCRE_SINGLELINE);
  try {
    if (RE.Search(sHtml)) do {
      sName=""; sLink=""; sYear=""; sImg="";
      HmsRegExMatch(gsPatternTitle, re.Match, sName);
      HmsRegExMatch(gsPatternLink , re.Match, sLink);
      HmsRegExMatch(gsPatternImg  , re.Match, sImg );
      HmsRegExMatch(gsPatternYear , re.Match, sYear);
      if (Trim(sLink)=="") continue;
      
      sLink = HmsExpandLink(sLink, gsUrlBase);
      sName = HmsHtmlToText(sName);
      HmsRegExMatch('^(.*?)/', sName, sName);
      if (sImg!="") sImg = HmsExpandLink(sImg , gsUrlBase); 
      
      if (Pos("${name}", sName)>0) continue;
      
      // Добавляем год в название, если установлен флаг и этого года в названии нет
      if ((gbYearInTitle && (Pos(sYear, sName)<1))) sName = Trim(sName)+" ("+sYear+")";
      
      if      (gsGroupingKey=="quant") {
        sGrp = Format("%.2d", [nGrp]);
        iCnt++; if (iCnt>=gnMaxInGroup) { nGrp++; iCnt=0; }
      } 
      else if (gsGroupingKey=="alph") sGrp = GetGroupName(sName);
      else if (gsGroupingKey=="year") sGrp = sYear;
      else sGrp = "";
      
      if (Trim(sGrp)!="") Folder = CreateFolder(FolderItem, sGrp);
      
      CreateFolder(Folder, sLink, sName, sYear, sImg);
      
    } while (RE.SearchAgain());
    
  } finally { RE.Free(); }
  
  if      (gsGroupingKey=="alph") FolderItem.Sort("mpTitle");
  else if (gsGroupingKey=="year") FolderItem.Sort("-mpTitle");
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
  sData = HmsDownloadURL('https://api.github.com/repos/WendyH/HMS-podcasts/contents/Tree.TV (wh)', "Accept-Encoding: gzip, deflate", true);
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

  if (Pos('--nocheckupdates', mpPodcastParameters) < 1) CheckPodcastUpdate(); // Проверка обновлений подкаста

  CheckParameters(); // Проверка установленных параметров подкаста

  CreateLinks();     // Загрузка страниц, поиск и создание ссылок

  HmsLogMessage(1, mpTitle+': Создано ссылок - '+IntToStr(gnItemsAdded));
}