// 2017.04.04
////////////////////////  Создание  списка  видео   ///////////////////////////
#define mpiJsonInfo 40032 // Идентификатор для хранения json информации о фильме
#define mpiKPID     40033 // Идентификатор для хранения ID кинопоиска

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
string    gsUrlBase     = "http://hdkinoteatr.com"; 
int       gnTotalItems  = 0; 
TDateTime gStart        = Now;
string    gsAPIUrl      = "http://api.lostcut.net/hdkinoteatr/";
int       gnDefaultTime = 6000;
string    gsTVDBInfo    = "";
bool gbUseSerialKPInfo  = false;
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
THmsScriptMediaItem CreateMediaItem(THmsScriptMediaItem Folder, string sTitle, string sLink, string sImg, int nTime, string sGrp='') {
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, Folder.ItemID, sGrp);
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
  Item[mpiFolderSortOrder] = "-mpCreateDate";
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Создание списка серий сериала с Moonwalk.cc
void CreateMoonwallkLinks(string sLink, string sKPID='') {
  string sData; TJsonObject PLAYLIST;
  // Пытаемся получить ID кинопоиска
  if (sKPID=='') {
    sKPID = Trim(FolderItem[mpiKPID]); // Получаем запомненный kinopoisk ID
    if (sKPID=='') HmsRegExMatch('/images/(film|film_big)/(\\d+)', mpThumbnail, sKPID, 2);
    if (sKPID=='') HmsRegExMatch('/iphone360_(\\d+)', mpThumbnail, sKPID);
  }
  
  sData = HmsDownloadURL(gsAPIUrl+'series?url='+HmsHttpEncode(sLink)+'&kpid='+sKPID, "Accept-Encoding: gzip, deflate", true);
  sData = HmsUtf8Decode(sData);
  PLAYLIST = TJsonObject.Create();
  try {
    PLAYLIST.LoadFromString(sData);
    CreateVideosFromJsonPlaylist(PLAYLIST.AsArray(), FolderItem);
  } finally { PLAYLIST.Free; }
}

///////////////////////////////////////////////////////////////////////////////
// Конвертирование строки даты вида ГГГГ-ММ-ДД в дату
TDateTime ConvertToDate(string sData) {
  string y, m, d, h, i, s;
  HmsRegExMatch3('(\\d+)-(\\d+)-(\\d+)', sData, y, m, d);
  HmsRegExMatch3('(\\d+):(\\d+):(\\d+)', sData, h, i, s);
  return StrToDateDef(d+'.'+m+'.'+y, Now);
}

///////////////////////////////////////////////////////////////////////////////
// Создание дерева ссылок из объекта Json
void CreateVideosFromJsonPlaylist(TJsonArray JARRAY, THmsScriptMediaItem Folder) {
  int i; string sName, sLink, sVal, sImg, sFmt; TJsonObject JOBJECT; THmsScriptMediaItem Item;
  
  if (JARRAY==nil) return;
  for (i=0; i<JARRAY.Length; i++) {
    sFmt = "%.2d %s"; if (JARRAY.Length > 99) sFmt = "%.3d %s";
    JOBJECT = JARRAY[i];
    if (JOBJECT.A["playlist"] != nil) {
      if (JARRAY.Length==1) { // Если сезон один, сразу создаём серии без папки
        CreateVideosFromJsonPlaylist(JOBJECT.A["playlist"], Folder);
      } else {
        sName = JOBJECT.S["comment"];
        Item  = Folder.AddFolder(sName);
        Item[mpiCreateDate] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0)); gnTotalItems++;
        CreateVideosFromJsonPlaylist(JOBJECT.A["playlist"], Item);
      }
    } else {
      sLink = JOBJECT.S["file"   ];
      sName = JOBJECT.S["comment"];
      sImg  = JOBJECT.S["image"  ]; if (sImg=='') sImg = mpThumbnail;
      if (sName=='Фильм') sName = mpTitle;
      if ((Pos('/serial/', sLink)>0) && (Pos('episode', sLink)<1))
        CreateFolder(Folder, sName, sLink, sImg);
      else if (Pos('youtu', sLink) > 0)
        CreateMediaItem(Folder, sName, sLink, sImg, 230);
      else {
        // Форматируем номер в два знака
        if (HmsRegExMatch("^(\\d+)", sName, sVal)) 
          sName = ReplaceStr(sName, sVal, Trim(Format(sFmt, [StrToInt(sVal), ""])));
        else
          sName = Format(sFmt, [i+1, sName]);
        Item = CreateMediaItem(Folder, sName, sLink, sImg, gnDefaultTime);
        FillVideoInfo(Item);
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
void FillVideoInfo(THmsScriptMediaItem Item) {
  TJsonObject VIDEO = TJsonObject.Create();
  try {
    VIDEO.LoadFromString(FolderItem[mpiJsonInfo]);
    Item[mpiYear    ] = VIDEO.I['year'     ];
    Item[mpiGenre   ] = VIDEO.S['genre'    ];
    Item[mpiDirector] = VIDEO.S['director' ];
    Item[mpiComposer] = VIDEO.S['composer' ];
    Item[mpiActor   ] = VIDEO.S['actors'   ];
    Item[mpiAuthor  ] = VIDEO.S['scenarist'];
    Item[mpiComment ] = VIDEO.S['translation'];
  } finally { VIDEO.Free; }
}

///////////////////////////////////////////////////////////////////////////////
void CreateLinksOfVideo() {
  string sData, sName, sLink, sID; TJsonObject VIDEO, PLAYLIST; THmsScriptMediaItem Item;
  VIDEO    = TJsonObject.Create();
  PLAYLIST = TJsonObject.Create();
  
  sData = Trim(FolderItem[mpiJsonInfo]);
  if (sData=='') {
    if (!HmsRegExMatch('.*/(\\d+)-', mpFilePath, sID)) return;
    sData = HmsDownloadURL(gsAPIUrl+'videos?id='+sID, "Accept-Encoding: gzip, deflate", true);
    sData = HmsUtf8Decode(sData);
  }
  
  try {
    VIDEO.LoadFromString(sData);
    if (VIDEO.IsType(jtArray)) VIDEO = VIDEO.AsArray[0];
    gStart = ConvertToDate(VIDEO.S['date']); // Устанавливаем началную точку для даты ссылок
    gnDefaultTime = VIDEO.I['time'];
    sLink = VIDEO.S['link'];
    sName = VIDEO.S['name'];
    if (VIDEO.B['isserial']) {
      if (VIDEO.S['name_eng']!='') mpSeriesTitle = VIDEO.S['name_eng'];
      else                         mpSeriesTitle = VIDEO.S['name'];
      HmsRegExMatch('^(.*?)\\s/\\s', mpSeriesTitle, mpSeriesTitle);
      FolderItem[mpiSeriesTitle] = mpSeriesTitle;
      FolderItem[mpiYear] = VIDEO.I['year'];
    }
    if (HmsRegExMatch('(\\[|\\{)', sLink, '')) {
      PLAYLIST.LoadFromString(sLink);
      CreateVideosFromJsonPlaylist(PLAYLIST.AsArray(), FolderItem);
    } else {
      
      if (HmsRegExMatch('/serial/', sLink, '')) {
        CreateMoonwallkLinks(sLink, VIDEO.S['kpid']); 
      } else {
        Item = CreateMediaItem(FolderItem, sName, sLink, mpThumbnail, gnDefaultTime);
        Item.CopyProperties(FolderItem.ItemParent, [mpiGenre,mpiDirector,mpiComposer,mpiActor,mpiAuthor,mpiYear,mpiComment]);
      }
      
    }
    
    // Специальная папка добавления/удаления в избранное
    if (VIDEO.B['isserial'] && (Pos('--controlfavorites', mpPodcastParameters)>0)) {
      Item = HmsFindMediaFolder(Podcast.ItemID, 'favorites');
      if (Item!=nil) {
        bool bExist = HmsFindMediaFolder(Item.ItemID, mpFilePath) != nil;
        if (bExist) { sName = "Удалить из избранного"; sLink = "-FavDel="+FolderItem.ItemParent.ItemID+";"+mpFilePath; }
        else        { sName = "Добавить в избранное" ; sLink = "-FavAdd="+FolderItem.ItemParent.ItemID+";"+mpFilePath; }
        CreateMediaItem(FolderItem, sName, sLink, '', 1, sName);
      }
    } 
    
  } finally { VIDEO.Free; PLAYLIST.Free; }
}

///////////////////////////////////////////////////////////////////////////////
// Создание списка видео
void CreateVideosList(string sParams='') {
  string sData, sLink, sName, sImg, sKPID, sYear, sVal, sGrp="", sGroupKey="";
  int i, nTime, iCnt=0, nGrp=0, nSortCount=0, nMaxInGroup=100, nMinInGroup=30;
  TJsonObject JSON, VIDEO; TJsonArray JARRAY; THmsScriptMediaItem Item, Folder=FolderItem;
  
  // Параметры
  bool bYearInTitle = Pos('--yearintitle', mpPodcastParameters) > 0; // Добавлять год к названию
  bool bNoFolders   = Pos('--nofolders'  , mpPodcastParameters) > 0; // Режим "без папкок" для фильмов
  bool bNumetare    = Pos('--numerate'   , mpPodcastParameters) > 0; // Нумеровать созданные ссылки
  HmsRegExMatch('--group=(\\w+)', mpPodcastParameters, sGroupKey);   // Режим группировки
  if (HmsRegExMatch('--maxingroup=(\\d+)', mpPodcastParameters, sVal)) nMaxInGroup = StrToInt(sVal);
  if (HmsRegExMatch('--miningroup=(\\d+)', mpPodcastParameters, sVal)) nMinInGroup = StrToInt(sVal);
  if (LeftCopy(sParams, 3)=='top') { bNoFolders=true; bNumetare=true; }
  
  // Если нет ссылки - формируем ссылку для поиска названия
  if ((Trim(mpFilePath)=='') || HmsRegExMatch('search="(.*?)"', mpFilePath, mpTitle)) {
    switch(FolderItem.ItemParent[mpiFilePath]) {
      case 'directors'   : { sParams='director=';    }
      case 'actors'      : { sParams='actor=';       }
      case 'scenarists'  : { sParams='scenarist=';   }
      case 'producers'   : { sParams='producer=';    }
      case 'composers'   : { sParams='composer=';    }
      case 'translations': { sParams='translation='; }
      default            : { sParams='q=';           }
    }
    sParams += HmsHttpEncode(HmsUtf8Encode(mpTitle));
  }
  
  // Получение данных и цикл создания ссылок на видео
  sData = HmsDownloadURL(gsAPIUrl+'videos?'+sParams, "Accept-Encoding: gzip, deflate", true);
  sData = HmsUtf8Decode(sData);
  JSON  = TJsonObject.Create();
  try {
    JSON.LoadFromString(sData);
    JARRAY = JSON.AsArray; if (JARRAY==nil) return;
    if ((sGroupKey=="") && (JARRAY.Length > nMaxInGroup)) sGroupKey="quant";
    else if (JARRAY.Length < nMinInGroup) sGroupKey=""; // Не группируем, если их мало
    for (i=0; i<JARRAY.Length; i++) {
      VIDEO = JARRAY[i]; sImg='';
      sName = VIDEO.S['name'];
      sLink = VIDEO.S['page'];
      sYear = VIDEO.S['year'];
      sKPID = VIDEO.S['kpid'];
      nTime = VIDEO.I['time']; if (nTime==0) nTime = 4800;
      sName = HmsJsonDecode(sName);
      if (Length(sKPID)>1) sImg = 'https://st.kp.yandex.net/images/film_iphone/iphone360_'+sKPID+'.jpg';
      // Добавляем год в название, если установлен флаг и этого года в названии нет
      if ((bYearInTitle && (Pos(sYear, sName)<1))) sName = Trim(sName)+" ("+sYear+")";
      if (bNumetare) sName = Format('%.3d %s', [i+1, sName]);
      
      switch (sGroupKey) {
        case "quant": { sGrp = Format("%.2d", [nGrp]); iCnt++; if (iCnt>=nMaxInGroup) { nGrp++; iCnt=0; } }
        case "alph" : { sGrp = GetGroupName(sName); }
        case "year" : { sGrp = sYear; }
        default     : { sGrp = "";    }
      }
      if (Trim(sGrp)!="") Folder = CreateFolder(FolderItem, sGrp, sGrp);
      
      if (bNoFolders && !VIDEO.B['isserial']) {
        Item = CreateMediaItem(Folder, sName, sLink, sImg, nTime);
        Item[mpiYear    ] = VIDEO.I['year'     ];
        Item[mpiGenre   ] = VIDEO.S['genre'    ];
        Item[mpiDirector] = VIDEO.S['director' ];
        Item[mpiComposer] = VIDEO.S['composer' ];
        Item[mpiActor   ] = VIDEO.S['actors'   ];
        Item[mpiAuthor  ] = VIDEO.S['scenarist'];
        Item[mpiComment ] = VIDEO.S['translation'];
      } else {
        Item = CreateFolder(Folder, sName, sLink, sImg);
        if (VIDEO.B['isserial']) {
          // Для сериалов запоминаем его название в свойстве mpiSeriesTitle
          if (VIDEO.S['name_eng']!='') mpSeriesTitle = VIDEO.S['name_eng'];
          else                         mpSeriesTitle = VIDEO.S['name'];
          // Если несколько вариантов ререз /, то берём только первый
          HmsRegExMatch('^(.*?)\\s/\\s', mpSeriesTitle, mpSeriesTitle);
          Item[mpiSeriesTitle] = mpSeriesTitle;
          Item[mpiYear       ] = VIDEO.I['year'];
        }
      }
      Item[mpiJsonInfo] = VIDEO.SaveToString(); // JSON информация о фильме
      Item[mpiKPID    ] = VIDEO.S['kpid'];      // ID с кинопоиск
    }
  } finally { JSON.Free; }
  if      (sGroupKey=="alph") FolderItem.Sort("mpTitle" );
  else if (sGroupKey=="year") FolderItem.Sort("-mpTitle");
}

///////////////////////////////////////////////////////////////////////////////
// Проверка и обновление скриптов подкаста
void CheckPodcastUpdate() {
  string sData, sName, sLang, sExt, sMsg; int i, mpiTimestamp=100602, mpiSHA, mpiScript;
  TJsonObject JSON, JFILE; TJsonArray JARRAY; bool bChanges=false;
  
  // Если после последней проверки прошло меньше получаса - валим
  if ((Trim(Podcast[550])=='') || (DateTimeToTimeStamp1970(Now, false)-StrToIntDef(Podcast[mpiTimestamp], 0) < 1800)) return;
  Podcast[mpiTimestamp] = DateTimeToTimeStamp1970(Now, false); // Запоминаем время проверки
  sData = HmsDownloadURL('https://api.github.com/repos/WendyH/HMS-podcasts/contents/HDKinoteatr', "Accept-Encoding: gzip, deflate", true);
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
} //Вызов в главной процедуре: if ((Pos('--nocheckupdates' , mpPodcastParameters)<1) && (mpComment=='--update')) CheckPodcastUpdate();

///////////////////////////////////////////////////////////////////////////////
// Проверка актуальности версии функции GetLink_Moonwalk в скриптах
void CheckMoonwalkFunction() {
  string sData, sFuncOld, sFuncNew; TJsonObject JSON; TDateTime UPDTIME;
  int nTimePrev, nTimeNow, mpiTimestamp=100862, mpiMWVersion=100821; 
  string sPatternMoonwalkFunction = "(// Получение ссылки с moonwalk.cc.*?// Конец функции поулчения ссылки с moonwalk.cc)";
  
  // Если после последней проверки прошло меньше получаса - валим
  if ((Trim(Podcast[550])=='') || (DateTimeToTimeStamp1970(Now, false)-StrToIntDef(Podcast[mpiTimestamp], 0) < 1800)) return;
  Podcast[mpiTimestamp] = DateTimeToTimeStamp1970(Now, false); // Запоминаем время проверки
  if (HmsRegExMatch(sPatternMoonwalkFunction, Podcast[550], sFuncOld, 1, PCRE_SINGLELINE)) {
    sData = HmsUtf8Decode(HmsDownloadURL("https://api.github.com/gists/3092dc755ad4a6c412e2fcd17c28d096", "Accept-Encoding: gzip, deflate", true));
    JSON  = TJsonObject.Create();
    try {
      JSON.LoadFromString(sData);
      sFuncNew = JSON.S['files\\GetLink_Moonwalk.cpp\\content'];
      if ((sFuncNew!='') && (Podcast[mpiMWVersion]!=JSON.S['updated_at'])) {
        HmsRegExMatch(sPatternMoonwalkFunction, sFuncNew, sFuncNew, 1, PCRE_SINGLELINE);
        Podcast[550] = ReplaceStr(Podcast[550], sFuncOld, sFuncNew); // Заменяем старую функцию на новую
        Podcast[mpiMWVersion] = JSON.S['updated_at'];
        HmsLogMessage(2, Podcast[mpiTitle]+": Обновлена функция получения ссылки с moonwalk.cc!");
      }
    } finally { JSON.Free; }
  }
  //Вызов в главной процедуре: if (Pos('--noupdatemoonwalk', mpPodcastParameters)<1) CheckMoonwalkFunction();
}

///////////////////////////////////////////////////////////////////////////////
//                     Г Л А В Н А Я   П Р О Ц Е Д У Р А                     //
{
  // Проверка на упоротых, которые пытаются запустить "Обновление подкастов" для всего подкаста разом
  if (InteractiveMode && (HmsCurrentMediaTreeItem.ItemClassName=='TVideoPodcastsFolderItem')) {
    HmsCurrentMediaTreeItem.DeleteChildItems(); // Дабы обновления всех подкастов не запустилось - удаляем их.
    ShowMessage('Обновление всех разделов разом запрещено!\nДля восстановления структуры\nзапустите "Создать ленты подкастов".');
    return;
  } 
  FolderItem.DeleteChildItems();

  if ((Pos('--nocheckupdates' , mpPodcastParameters)<1) && (mpComment=='--update')) CheckPodcastUpdate(); // Проверка обновлений подкаста
  if (Pos('--noupdatemoonwalk', mpPodcastParameters)<1) CheckMoonwalkFunction();
 
  if      (Pos('/serial/', mpFilePath) > 0) CreateMoonwallkLinks(mpFilePath); // вариант перевода сериала
  else if (LeftCopy(mpFilePath, 4)=='http') CreateLinksOfVideo(); // создание ссылок на серии в избранном
  else CreateVideosList(mpFilePath);
  
  HmsLogMessage(1, Podcast[mpiTitle]+' "'+mpTitle+'": Создано элементов - '+IntToStr(gnTotalItems));
}