// 2018.03.24
///////////////////////  Создание структуры подкаста  /////////////////////////
#define mpiJsonInfo 40032 // Идентификатор для хранения json информации о фильме

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
int       gnTotalItems = 0; 
TDateTime gStart       = Now;
string    gsAPIUrl     = "http://api.lostcut.net/hdgo/";
string    gsToken      = "Q298nQsLY481iJzUPrlwVnRh6EqC8Ctd";

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

///////////////////////////////////////////////////////////////////////////////
// Установка переменной Podcast: поиск родительской папки, содержащий скрипт
THmsScriptMediaItem GetRoot() {
  Podcast = FolderItem; // Начиная с текущего элемента, ищется создержащий срипт
  while ((Trim(Podcast[550])=='') && (Podcast[532]!='1') && (Podcast.ItemParent!=nil)) 
    Podcast=Podcast.ItemParent;
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
// Создание папки или подкаста
THmsScriptMediaItem CreateFolder(THmsScriptMediaItem ParentFolder, string sName, string sLink, string sImg='') {
  THmsScriptMediaItem Item = ParentFolder.AddFolder(sLink); // Создаём папку с указанной ссылкой
  Item[mpiTitle     ] = sName; // Присваиваем наименование
  Item[mpiThumbnail ] = sImg;  // Картинка
  Item[mpiCreateDate] = DateTimeToStr(IncTime(gStart, 0, -gnTotalItems, 0, 0)); // Для обратной сортировки по дате создания
  Item[mpiFolderSortOrder] = -mpiCreateDate;
  gnTotalItems++;              // Увеличиваем счетчик созданных элементов
  return Item;                 // Возвращаем созданный объект
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки на видео
THmsScriptMediaItem CreateMediaItem(THmsScriptMediaItem Folder, string sTitle, string sLink, string sImg="", string sGrp="", int nTime=0) {
  if (nTime==0) nTime = 6600;
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, Folder.ItemID, sGrp);
  Item[mpiTitle     ] = sTitle;
  Item[mpiThumbnail ] = sImg;
  Item[mpiCreateDate] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0));
  Item[mpiTimeLength] = HmsTimeFormat(nTime)+".000";
  gnTotalItems++;
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки-ошибки
void ErrorItem(string sMsg) {
  THmsScriptMediaItem Item = HmsCreateMediaItem('Err'+IntToStr(FolderItem.ChildCount), FolderItem.ItemID);
  Item[mpiTitle     ] = sMsg;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/icons/symbol-error.png';
}

///////////////////////////////////////////////////////////////////////////////
// Создание информационной ссылки
void InfoItem(string sName, string sVal) {
  if ((sVal=="") || (sVal=="-")) return; if (sName!="") sName+= ": ";
  THmsScriptMediaItem Item = HmsCreateMediaItem('Info'+IntToStr(FolderItem.ChildCount), FolderItem.ItemID);
  Item[mpiTitle     ] = sName+sVal;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/vids/info.jpg';
  Item[mpiTimeLength] = 20;
  Item[mpiCreateDate] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0));
  gnTotalItems++;
}

///////////////////////////////////////////////////////////////////////////////
string DetectQualityIndex() {
  string sQual, sSelectedQual="", sVal;
  HmsRegExMatch("--quality=(\\w+)", mpPodcastParameters, sQual);
  if      (sQual=="low"   ) sSelectedQual = "?q=1";
  else if (sQual=="medium") sSelectedQual = "?q=3";
  else if (HmsRegExMatch("(\\d+)", sQual, sVal)) {
    extended minDiff = 999999; // search nearest quality
    Variant  aQualList = [360,480,720,1080];
    for (int i=0; i < Length(aQualList); i++) {
      extended diff = aQualList[i] - StrToInt(sVal);
      if (Abs(diff) < minDiff) {
        minDiff = Abs(diff);
        sSelectedQual = "?q="+IntToStr(i+1);
      }
    }
  } else sSelectedQual = "";
  return sSelectedQual;
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылок на список переводов или сезонов с сериями с hdgo.cc
void CreateHdgoLinks(string sLink) {
  string sData, sName, sID, sVal, sGrp, sQual, sImg, sSelectedQual;
  THmsScriptMediaItem Item; int i, n, nTime;
  TJsonObject JSON, HDGO, SEASON, EPISODE; TJsonArray SEASONS, EPISODES; 
  
  if (!HmsRegExMatch(".*/(\\d+)", sLink, sID)) return;
  
  // Дополнительные параметры
  HmsRegExMatch("--quality=(\\w+)", mpPodcastParameters, sQual);
  bool bDirectLinks = (Pos("--directlinks", mpPodcastParameters)>0);
  
  JSON = TJsonObject.Create();
  HDGO = TJsonObject.Create();
  try {
    JSON.LoadFromString(FolderItem[mpiJsonInfo]);
    nTime = JSON.I["time"];
    if (JSON.B["isserial"]) {
      sData = HmsUtf8Decode(HmsDownloadURL(gsAPIUrl+"series?id="+JSON.S["id"]));
      HDGO.LoadFromString(sData);
      SEASONS = HDGO.AsArray;
      bool bNoSeasons = (SEASONS.Length==1) && (SEASONS[0].S["name"]=="Сезон 01");
      for (i=0; i < SEASONS.Length; i++) {
        SEASON = SEASONS[i];
        if (bNoSeasons) sGrp = "";
        else            sGrp = SEASON.S["name"];
        EPISODES = SEASON.A["episodes"];
        for (n=0; n < EPISODES.Length; n++) {
          EPISODE = EPISODES[n];
          sLink = EPISODE.S["link" ];
          sName = EPISODE.S["name" ];
          sImg  = EPISODE.S["image"];
          if (bDirectLinks)
            sLink = GetMP4LinkHdgo(sLink);
          CreateMediaItem(FolderItem, sName, sLink, sImg, sGrp, nTime);
        }
      }
      
    } else {
      sName = JSON.S["name"];
      sLink = mpFilePath;
      if (sQual=="all") {
        sData = LoadHdgoHtml(sLink, "");
        TRegExpr RE = TRegExpr.Create("url:\\s*'(.*?)'", PCRE_SINGLELINE);
        if (RE.Search(sData)) do {
          sLink = RE.Match(); sVal="";
          HmsRegExMatch("/(\\d)/", sLink, sVal);
          if      (sVal=="1") sName = "[360p] "+JSON.S["name"];
          else if (sVal=="2") sName = "[480p] "+JSON.S["name"];
          else if (sVal=="3") sName = "[720p] "+JSON.S["name"];
          else if (sVal=="4") sName = "[1080p] "+JSON.S["name"];
          sLink = GetMP4LinkHdgo(sLink, true);
          CreateMediaItem(FolderItem, sName, sLink, mpThumbnail, "", nTime);
        } while (RE.SearchAgain); else {
          ErrorItem("Video not found!");
        }
        
      } else {
        sSelectedQual = DetectQualityIndex();
        sLink += sSelectedQual;
        if (bDirectLinks)
          sLink = GetMP4LinkHdgo(sLink);
        CreateMediaItem(FolderItem, sName, sLink, mpThumbnail, "", nTime);
      }
    }    
  } finally { JSON.Free; HDGO.Free; }
}

///////////////////////////////////////////////////////////////////////////////
string LoadHdgoHtml(string sLink, string &sReferer) {
  string sHtml; HmsRegExMatch("(https?://.*?/)", sLink, sReferer);
  sHtml = HmsDownloadURL(sLink, "Referer: http://hdgo.club");
  if (HmsRegExMatch('<iframe[^>]+src="(.*?)"', sHtml, sLink)) {
    if (LeftCopy(sLink, 2)=="//") sLink = "http:"+Trim(sLink);
    HmsRegExMatch("(https?://.*?/)", sLink, sReferer);
    sHtml = HmsDownloadURL(sLink, "Referer: http://hdgo.club");
  }
  if (!HmsRegExMatch("url:\\s*'http", sHtml, '') && HmsRegExMatch('<iframe[^>]+src="(.*?)"', sHtml, sLink)) {
    if (LeftCopy(sLink, 2)=="//") sLink = "http:"+Trim(sLink);
    HmsRegExMatch("(https?://.*?/)", sLink, sReferer);
    sHtml = HmsDownloadURL(sLink, "Referer: "+sLink);
  }
  return sHtml;
}

///////////////////////////////////////////////////////////////////////////////
string GetMP4LinkHdgo(string sLink, bool bNotLoadHtml = false) {
  string sHtml, sServer, sPage, sRet, sMp4Link, sQual, sReferer;
  HmsRegExMatch("(https?://.*?/)", sLink, sReferer);
  if (!HmsRegExMatch("q=(\\d)", sLink, sQual)) {
    sQual = DetectQualityIndex();
    HmsRegExMatch("q=(\\d)", sQual, sQual);
  }
  int INTERNET_FLAG_NO_AUTO_REDIRECT = 0x00200000;
  if (bNotLoadHtml) {
    sMp4Link = sLink;
  } else {
    sMp4Link = ""; sHtml = LoadHdgoHtml(sLink, sReferer);
    if (sQual!="") HmsRegExMatch("url:\\s*'([^']+/"+sQual+"/.*?)'", sHtml, sMp4Link, 1, PCRE_SINGLELINE);
    if (sMp4Link=="") HmsRegExMatch(".*url:\\s*'(.*?)'", sHtml, sMp4Link, 1, PCRE_SINGLELINE);
  }
  HmsRegExMatch2("//(.*?)(/.*)", sMp4Link, sServer, sPage);
  HmsSendRequestEx(sServer, sPage, "HEAD", "", sReferer, "", 80, INTERNET_FLAG_NO_AUTO_REDIRECT, sRet, true);
  HmsRegExMatch("Location:(.*?)[\r\n]", sRet, sLink);
  return sLink;
}

///////////////////////////////////////////////////////////////////////////////
void CreateInfoLinks() {
  string sID, sImg; int nTime = 210;
  // Создание информационных ссылок
  if (Pos('--infoitems', mpPodcastParameters) > 0) {
    TJsonObject VIDEO = TJsonObject.Create();
    try {
      VIDEO.LoadFromString(FolderItem[mpiJsonInfo]);
      if (VIDEO.B["kpid" ]) sImg = 'https://st.kp.yandex.net/images/film_iphone/iphone360_'+VIDEO.S["kpid"]+'.jpg';
      if (VIDEO.B["image"]) sImg = 'https://st.kp.yandex.net/images/'+VIDEO.S["image"];
      if (VIDEO.B["trailer_hd" ]) CreateMediaItem(FolderItem, 'Трейлер HD' , VIDEO.S["trailer_hd" ], sImg, "", nTime);
      if (VIDEO.B["trailer_sd" ]) CreateMediaItem(FolderItem, 'Трейлер SD' , VIDEO.S["trailer_sd" ], sImg, "", nTime);
      if (!VIDEO.B["trailer_hd"] && !VIDEO.B["trailer_sd"] && VIDEO.B["yt"]) {
        sImg = "https://img.youtube.com/vi/"+VIDEO.S["yt"]+"/sddefault.jpg";
        CreateMediaItem(FolderItem, 'Трейлер (youtube)', "https://www.youtube.com/watch?v="+VIDEO.S["yt"], sImg, "", nTime);
      }
      InfoItem('Страна'  , VIDEO.S["country" ]);
      InfoItem('Жанр'    , VIDEO.S["genre"   ]);
      InfoItem('Год'     , VIDEO.S["year"    ]);
      InfoItem('Режиссёр', VIDEO.S["director"]);
      if (VIDEO.B["rating_kp"  ]) InfoItem('Рейтинг КП'  , VIDEO.S["rating_kp"  ]+' ('+VIDEO.S["rating_kp_votes"  ]+')');
      if (VIDEO.B["rating_imdb"]) InfoItem('Рейтинг IMDb', VIDEO.S["rating_imdb"]+' ('+VIDEO.S["rating_imdb_votes"]+')');
      InfoItem('Перевод' , VIDEO.S["translator"]);
      InfoItem('Качество', VIDEO.S["quality"   ]);
    } finally { VIDEO.Free; }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Создание списка видео
void CreateVideosList(string sParams="") {
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
  if (Pos('top=', sParams)>0) { bNoFolders=true; bNumetare=true; }
  
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
    sParams += HmsHttpEncode(HmsUtf8Encode(mpTitle))+"&serials=2";
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
      sLink = "http://hdgo.club/video/"+gsToken+"/"+VIDEO.S['id']+"/";
      sYear = VIDEO.S['year'];
      sKPID = VIDEO.S['kpid'];
      sName = HmsJsonDecode(sName);
      if (Length(sKPID)>1) sImg = 'https://st.kp.yandex.net/images/film_iphone/iphone360_'+sKPID+'.jpg';
      // Добавляем год в название, если установлен флаг и этого года в названии нет
      if ((bYearInTitle && (sYear!="0") && (Pos(sYear, sName)<1))) sName = Trim(sName)+" ("+sYear+")";
      if (bNumetare) sName = Format('%.3d %s', [i+1, sName]);
      
      switch (sGroupKey) {
        case "quant": { sGrp = Format("%.2d", [nGrp]); iCnt++; if (iCnt>=nMaxInGroup) { nGrp++; iCnt=0; } }
        case "alph" : { sGrp = GetGroupName(sName); }
        case "year" : { sGrp = sYear; }
        default     : { sGrp = "";    }
      }
      if (Trim(sGrp)!="") { Folder = CreateFolder(FolderItem, sGrp, sGrp); gnTotalItems--; }
      
      if (bNoFolders && !VIDEO.B['isserial']) {
        Item = CreateMediaItem(Folder, sName, sLink, sImg, "", VIDEO.I['time']);
        Item[mpiYear    ] = VIDEO.I['year'      ];
        Item[mpiGenre   ] = VIDEO.S['genre'     ];
        Item[mpiDirector] = VIDEO.S['director'  ];
        Item[mpiActor   ] = VIDEO.S['actors'    ];
        Item[mpiComment ] = VIDEO.S['translator'];
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
    }
  } finally { JSON.Free; }
  if      (sGroupKey=="alph") FolderItem.Sort("mpTitle" );
  else if (sGroupKey=="year") FolderItem.Sort("-mpTitle");
  HmsLogMessage(1, Podcast[mpiTitle]+"/"+mpTitle+': Создано ссылок - '+IntToStr(gnTotalItems));
}

///////////////////////////////////////////////////////////////////////////////
// Проверка и обновление скриптов подкаста
void CheckPodcastUpdate() {
  string sData, sName, sLang, sExt, sMsg; int i, mpiTimestamp=100602, mpiSHA, mpiScript;
  TJsonObject JSON, JFILE; TJsonArray JARRAY; bool bChanges=false;
  
  // Если после последней проверки прошло меньше получаса - валим
  if ((Trim(Podcast[550])=='') || (DateTimeToTimeStamp1970(Now, false)-StrToIntDef(Podcast[mpiTimestamp], 0) < 14400)) return; // раз в 4 часа
  Podcast[mpiTimestamp] = DateTimeToTimeStamp1970(Now, false); // Запоминаем время проверки
  sData = HmsDownloadURL('https://api.github.com/repos/WendyH/HMS-podcasts/contents/hdgo', "Accept-Encoding: gzip, deflate", true);
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
//                     Г Л А В Н А Я   П Р О Ц Е Д У Р А                     //
{ string sYear;
  
  // Проверка на упоротых, которые пытаются запустить "Обновление подкастов" для всех сериалов разом
  if (InteractiveMode && (HmsCurrentMediaTreeItem.ItemClassName=='TVideoPodcastsFolderItem')) {
    HmsCurrentMediaTreeItem.DeleteChildItems(); // Дабы обновления всех подкастов не запустилось - удаляем их.
    ShowMessage('Обновление всех разделов разом запрещено!\nДля восстановления структуры\nзапустите "Создать ленты подкастов".');
    return;
  } 
  FolderItem.DeleteChildItems();
 
  if ((Pos('--nocheckupdates' , mpPodcastParameters)<1) && (mpComment=='--update')) CheckPodcastUpdate(); // Проверка обновлений подкаста
    
  if (mpFilePath=="lastvideos") {
    mpPodcastParameters += " --maxingroup=200";
    CreateVideosList("serials=0&limit=200&category=-24");
    
  } else if (HmsRegExMatch("films-year=(\\d+)", mpFilePath, sYear)) {
    CreateVideosList("year="+sYear);
    
  } else if (Pos("hdgo.", mpFilePath)>0) {
    CreateHdgoLinks(mpFilePath);
    THmsScriptMediaItem Item = HmsFindMediaFolder(Podcast.ItemID, 'favorites');
    if (Item!=nil) {
      string sName, sLink;
      bool bExist = HmsFindMediaFolder(Item.ItemID, mpFilePath) != nil;
      if (bExist) { sName = "Удалить из избранного"; sLink = "-FavDel="+FolderItem.ItemParent.ItemID+";"+mpFilePath; }
      else        { sName = "Добавить в избранное" ; sLink = "-FavAdd="+FolderItem.ItemParent.ItemID+";"+mpFilePath; }
      CreateMediaItem(FolderItem, sName, sLink, 'http://wonky.lostcut.net/icons/ok.png', sName, 60);
    }
    CreateInfoLinks();

  } else {
    CreateVideosList(mpFilePath);
  }
  
}