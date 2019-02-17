// 2018.09.27
////////////////////  Получение ссылки на медиа-ресурс  ///////////////////////
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
  Podcast = PodcastItem; // Начиная с текущего элемента, ищется создержащий срипт
  while ((Trim(Podcast[550])=='') && (Podcast[532]!='1') && (Podcast.ItemParent!=nil)) 
    Podcast=Podcast.ItemParent;
  return Podcast;
}

///////////////////////////////////////////////////////////////////////////////
// Добавление или удаление сериала из/в избранное
void AddRemoveFavorites() {
  string sLink, sCmd, sID; THmsScriptMediaItem FavFolder, Item, Src; bool bExist;
  
  if (!HmsRegExMatch3('(Add|Del)=(.*?);(.*)', mpFilePath, sCmd, sID, sLink)) return;
  FavFolder = HmsFindMediaFolder(Podcast.ItemID, 'favorites');
  if (FavFolder!=nil) {
    Item = HmsFindMediaFolder(FavFolder.ItemID, sLink);
    bExist = (Item != nil);
    if      ( bExist && (sCmd=='Del')) Item.Delete();
    else if (!bExist && (sCmd=='Add')) {
      Src = HmsFindMediaFolder(sID, sLink);
      if (Src!=nil) {
        Item = FavFolder.AddFolder(Src[mpiFilePath]);
        Item.CopyProperties(Src, [mpiTitle, mpiThumbnail, mpiJsonInfo, mpiYear]);
      }
    }
  }
  MediaResourceLink = ProgramPath()+'\\Presentation\\Images\\videook.mp4';
  if (bExist) { mpTitle = "Добавить в избранное" ; mpFilePath = "-FavDel="+sID+";"+sLink; }
  else        { mpTitle = "Удалить из избранного"; mpFilePath = "-FavDel="+sID+";"+sLink; }
  PodcastItem[mpiTitle] = mpTitle;
  PodcastItem.ItemOrigin.ItemParent[mpiTitle] = mpTitle;
  PodcastItem[mpiFilePath] = mpFilePath;
  PodcastItem.ItemOrigin.ItemParent[mpiFilePath] = mpFilePath;
  HmsIncSystemUpdateID(); // Говорим устройству об обновлении содержания
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
  THmsScriptMediaItem Item = HmsCreateMediaItem('Err'+IntToStr(PodcastItem.ChildCount), PodcastItem.ItemID);
  Item[mpiTitle     ] = sMsg;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/icons/symbol-error.png';
}

///////////////////////////////////////////////////////////////////////////////
// Создание информационной ссылки
void InfoItem(string sName, string sVal) {
  if ((sVal=="") || (sVal=="-")) return; if (sName!="") sName+= ": ";
  THmsScriptMediaItem Item = HmsCreateMediaItem('Info'+IntToStr(PodcastItem.ChildCount), PodcastItem.ItemID);
  Item[mpiTitle     ] = sName+sVal;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/vids/info.jpg';
  Item[mpiTimeLength] = 20;
  Item[mpiCreateDate] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0));
  Item[mpiTranscodingActive ] = true;
  Item[mpiTranscodingProfile] = "Фильмы (основной)";
  gnTotalItems++;
}

///////////////////////////////////////////////////////////////////////////////
// Формирование видео с картинкой с информацией о фильме
bool VideoPreview() {
  string sFileImage, sID="", sCacheDir=IncludeTrailingBackslash(HmsTempDirectory)+'hdgo/';;
  TJsonObject VIDEO = TJsonObject.Create();
  try {
    VIDEO.LoadFromString(PodcastItem.ItemParent[mpiJsonInfo]);
    sID = VIDEO.S["id"];
  } finally { VIDEO.Free; }
  if (sID!="") {
    ForceDirectories(sCacheDir);
    sFileImage = ExtractShortPathName(sCacheDir)+'videopreview_'; // Файл-заготовка для сохранения картинки
    HmsDownloadURLToFile(gsAPIUrl+"infopic?id="+sID, sFileImage);
    // Копируем и нумеруем файл картики столько раз, сколько секунд мы будем её показывать
    for (int n=1; n<=20; n++) CopyFile(sFileImage, sFileImage+Format('%.3d.jpg', [n]), false);
    // Для некоторых телевизоров (Samsung) видео без звука отказывается проигрывать, поэтому скачиваем звук тишины
    string sFileMP3 = ExtractShortPathName(HmsTempDirectory)+'\\silent.mp3';
    try {
      if (!FileExists(sFileMP3)) HmsDownloadURLToFile('http://wonky.lostcut.net/mp3/silent.mp3', sFileMP3);
      sFileMP3 = '-i "'+sFileMP3+'"';
    } except { sFileMP3=''; }
    // Формируем из файлов пронумерованных картинок и звукового команду для формирования видео
    MediaResourceLink = Format('%s -f image2 -r 1 -i "%s" -c:v libx264 -pix_fmt yuv420p ', [sFileMP3, sFileImage+'%03d.jpg']);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Получение ссылки на Youtube
void GetLink_Youtube31(string sLink) {
  string sData, sVideoID='', sMaxHeight='', sAudio='', sSubtitlesLanguage='ru',
  sSubtitlesUrl, sFile, sVal, sMsg, sConfig, sHeaders, ttsDef; 
  TJsonObject JSON; TRegExpr RegEx;
  
  sHeaders = 'Referer: '+sLink+#13#10+
             'User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.86 Safari/537.36'+#13#10+
             'Origin: http://www.youtube.com'+#13#10;
  
  HmsRegExMatch('--maxheight=(\\d+)'    , mpPodcastParameters, sMaxHeight);
  HmsRegExMatch('--sublanguage=(\\w{2})', mpPodcastParameters, sSubtitlesLanguage);
  bool bSubtitles = (Pos('--subtitles'  , mpPodcastParameters)>0);  
  bool bAdaptive  = (Pos('--adaptive'   , mpPodcastParameters)>0);  
  bool bNotDE     = (Pos('notde=1'      , sLink)>0);  
  
  if (!HmsRegExMatch('[\\?&]v=([^&]+)'       , sLink, sVideoID))
    if (!HmsRegExMatch('youtu.be/([^&]+)'      , sLink, sVideoID))
    HmsRegExMatch('/(?:embed|v)/([^\\?]+)', sLink, sVideoID);
  
  if (sVideoID=='') { HmsLogMessage(2, 'Невозможно получить Video ID в ссылке Youtube'); return; }
  
  sLink = 'http://www.youtube.com/watch?v='+sVideoID+'&hl=ru&persist_hl=1&has_verified=1';
  
  sData = HmsDownloadURL(sLink, sHeaders, true);
  sData = HmsRemoveLineBreaks(sData);
  // Если еще не установлена реальная длительность видео - устанавливаем
  if ((Trim(mpTimeLength)=='') || (RightCopy(mpTimeLength, 6)=='00.000')) {
    if (HmsRegExMatch2('itemprop="duration"[^>]+content="..(\\d+)M(\\d+)S', sData, sVal, sMsg)) {
      PodcastItem[mpiTimeLength] = StrToInt(sVal)*60+StrToInt(sMsg);
    } else {
      sVal = HmsDownloadURL('http://www.youtube.com/get_video_info?html5=1&c=WEB&cver=html5&cplayer=UNIPLAYER&hl=ru&video_id='+sVideoID, sHeaders, true);
      if (HmsRegExMatch('length_seconds=(\\d+)', sData, sMsg))
        PodcastItem[mpiTimeLength] = StrToInt(sMsg);
    }
  }
  if (!HmsRegExMatch('player.config\\s*?=\\s*?({.*?});', sData, sConfig)) {
    // Если в загруженной странице нет нужной информации, пробуем немного по-другому
    sLink = 'http://hms.lostcut.net/youtube/g.php?v='+sVideoID;
    if (sMaxHeight!=''                  ) sLink += '&max_height='+sMaxHeight;
    if (Trim(mpPodcastMediaFormats )!='') sLink += '&media_formats='+mpPodcastMediaFormats;
    if (bAdaptive                       ) sLink += '&adaptive=1';
    sData = HmsUtf8Decode(HmsDownloadUrl(sLink));
    if (HmsRegExMatch('"reason":"(.*?)"' , sData, sMsg)) { 
      HmsLogMessage(2 , sMsg); 
      return; 
    } else {
      sData = HmsJsonDecode(sData);
      HmsRegExMatch('"url":"(.*?)"', sData, MediaResourceLink);
    }
  }
  
  String hlsUrl, ttsUrl, flp, jsUrl, dashMpdLink, streamMap, playerId, algorithm;
  String sType, itag, sig, alg, s;
  String UrlBase = "";
  int  i, n, w, num, height, priority, minPriority = 90, selHeight, maxHeight = 1080;
  bool is3D; 
  TryStrToInt(sMaxHeight, maxHeight);
  JSON = TJsonObject.Create();
  try {
    JSON.LoadFromString(sConfig);
    hlsUrl      = HmsExpandLink(JSON.S['args\\hlsvp' ], UrlBase);
    ttsUrl      = HmsExpandLink(JSON.S['args\\caption_tracks'], UrlBase);
    flp         = HmsExpandLink(JSON.S['url'         ], UrlBase);
    jsUrl       = HmsExpandLink(JSON.S['assets\\js'  ], UrlBase);
    streamMap   = JSON.S['args\\url_encoded_fmt_stream_map'];
    if (bAdaptive && JSON.B['args\\adaptive_fmts']) 
      streamMap = JSON.S['args\\adaptive_fmts'];
    if ((streamMap=='') && (hlsUrl=='')) {
      sMsg = "Невозможно найти данные для воспроизведения на странице видео.";
      if (HmsRegExMatch('(<h\\d[^>]+class="message".*?</h\\d>)', sData, sMsg)) sMsg = HmsUtf8Decode(HmsHtmlToText(sMsg));
      HmsLogMessage(2, sMsg);
      return;
    }
  } finally { JSON.Free; }
  if (Copy(jsUrl, 1, 2)=='//') jsUrl = 'http:'+Trim(jsUrl);
  HmsRegExMatch('/player-([\\w_-]+)/', jsUrl, playerId);
  algorithm = HmsDownloadURL('https://hms.lostcut.net/youtube/getalgo.php?jsurl='+HmsHttpEncode(jsUrl));
  
  if (hlsUrl!='') {
    MediaResourceLink = ' '+hlsUrl;
    
    sData = HmsDownloadUrl(sLink, sHeaders, true);
    RegEx = TRegExpr.Create('BANDWIDTH=(\\d+).*?RESOLUTION=(\\d+)x(\\d+).*?(http[^#]*)', PCRE_SINGLELINE);
    try {
      if (RegEx.Search(sData)) do {
        sLink = '' + RegEx.Match(4);
        height = StrToIntDef(RegEx.Match(3), 0);
        if (mpPodcastMediaFormats!='') {
          priority = HmsMediaFormatPriority(height, mpPodcastMediaFormats);
          if ((priority>=0) && (priority>minPriority)) {
            MediaResourceLink = sLink; minPriority = priority;
          }
        } else if ((height > selHeight) && (height <= maxHeight)) {
          MediaResourceLink = sLink; selHeight = height;
        }
      } while (RegEx.SearchAgain());
    } finally { RegEx.Free(); }
    
  } else if (streamMap!='') {
    i=1; while (i<=Length(streamMap)) {
      sData = Trim(ExtractStr(streamMap, ',', i));
      sType = HmsHttpDecode(ExtractParam(sData, 'type', '', '&'));
      itag  = ExtractParam(sData, 'itag'    , '', '&');
      is3D  = ExtractParam(sData, 'stereo3d', '', '&') == '1';
      sLink = '';
      if (Pos('url=', sData)>0) {
        sLink = ' ' + HmsHttpDecode(ExtractParam(sData, 'url', '', '&'));
        if (Pos('&signature=', sLink)==0) {
          sig = HmsHttpDecode(ExtractParam(sData, 'sig', '', '&'));    
          if (sig=='') {
            sig = HmsHttpDecode(ExtractParam(sData, 's', '', '&'));
            for (w=1; w<=WordCount(algorithm, ' '); w++) {
              alg = ExtractWord(w, algorithm, ' ');
              if (Length(alg)<1) continue;
              if (Length(alg)>1) TryStrToInt(Copy(alg, 2, 4), num);
              if (alg[1]=='r') {s=''; for(n=Length(sig); n>0; n--) s+=sig[n]; sig = s;   } // Reverse
              if (alg[1]=='s') {sig = Copy(sig, num+1, Length(sig));                     } // Clone
              if (alg[1]=='w') {n = (num-Trunc(num/Length(sig)))+1; Swap(sig[1], sig[n]);} // Swap
            }
          }
          if (sig!='') sLink += '&signature=' + sig;
        }
      }
      if (itag in ([139,140,141,171,172])) { sAudio = sLink; continue; }
      if (sLink!='') {
        height = 0; //http://www.genyoutube.net/formats-resolution-youtube-videos.html
        if      (itag in ([13,17,160                  ])) height = 144;
        else if (itag in ([5,36,92,132,133,242        ])) height = 240;
        else if (itag in ([6                          ])) height = 270;
        else if (itag in ([18,34,43,82,100,93,134,243 ])) height = 360;
        else if (itag in ([35,44,83,101,94,135,244,43 ])) height = 480;
        else if (itag in ([22,45,84,102,95,136,298,247])) height = 720;
        else if (itag in ([37,46,85,96,137,248,299    ])) height = 1080;
        else if (itag in ([264,271                    ])) height = 1440;
        else if (itag in ([266,138                    ])) height = 2160;
        else if (itag in ([272                        ])) height = 2304;
        else if (itag in ([38                         ])) height = 3072;
        else continue;
        if (mpPodcastMediaFormats!='') {
          priority = HmsMediaFormatPriority(height, mpPodcastMediaFormats);
          if ((priority>=0) || (priority<minPriority)) {
            MediaResourceLink = sLink; minPriority = priority; selHeight = height;
          }
        } else if ((height>selHeight) && (height<= maxHeight)) {
          MediaResourceLink = sLink; selHeight = height;
          
        } else if ((height>=selHeight) && (height<= maxHeight) && (itag in ([18,22,37,38,82,83,84,85]))) {
          // Если выоста такая же, но формат MP4 - то выбираем именно его (делаем приоритет MP4)
          MediaResourceLink = sLink; selHeight = height;
        }
      }
    }
    if (bAdaptive && (sAudio!='')) MediaResourceLink = '-i "'+Trim(MediaResourceLink)+'" -i "'+Trim(sAudio)+'"';
    
  }
  // Если есть субтитры и в дополнительных параметрах указано их показывать - загружаем 
  if (bSubtitles && (ttsUrl!='')) {
    string sTime1, sTime2; float nStart, nDur;
    sLink = ''; n = WordCount(ttsUrl, ',');
    for (i=1; i <= n; i++) {
      sData = ExtractWord(i, ttsUrl, ',');
      sType = HmsPercentDecode(ExtractParam(sData, 'lc', '', '&'));
      sVal  = HmsPercentDecode(ExtractParam(sData, 'u' , '', '&'));
      if (sType == 'en') sLink = sVal;
      if (sType == sSubtitlesLanguage) { sLink = sVal; break; }
    }
    if (sLink != '') {
      sData = HmsDownloadURL(sLink, sHeaders, true);
      sMsg  = ''; i = 0;
      RegEx = TRegExpr.Create('(<(text|p).*?</(text|p)>)', PCRE_SINGLELINE); // Convert to srt format
      try {
        if (RegEx.Search(sData)) do {
          if      (HmsRegExMatch('start="([\\d\\.]+)', RegEx.Match, sVal)) nStart = StrToFloat(ReplaceStr(sVal, '.', ','))*1000;
          else if (HmsRegExMatch('t="(\\d+)'         , RegEx.Match, sVal)) nStart = StrToFloat(sVal);
          if      (HmsRegExMatch('dur="([\\d\\.]+)'  , RegEx.Match, sVal)) nDur   = StrToFloat(ReplaceStr(sVal, '.', ','))*1000;
          else if (HmsRegExMatch('d="(\\d+)'         , RegEx.Match, sVal)) nDur   = StrToFloat(sVal);
          sTime1 = HmsTimeFormat(Int(nStart/1000))+','+RightCopy(Str(nStart), 3);
          sTime2 = HmsTimeFormat(Int((nStart+nDur)/1000))+','+RightCopy(Str(nStart+nDur), 3);
          sMsg += Format("%d\n%s --> %s\n%s\n\n", [i, sTime1, sTime2, HmsHtmlToText(HmsHtmlToText(RegEx.Match(0), 65001))]);
          i++;
        } while (RegEx.SearchAgain());
      } finally { RegEx.Free(); }
      sFile = HmsSubtitlesDirectory+'\\Youtube\\'+PodcastItem.ItemID+'.'+sSubtitlesLanguage+'.srt';
      HmsStringToFile(sMsg, sFile);
      PodcastItem[mpiSubtitleLanguage] = sFile;
    }
  }
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
    JSON.LoadFromString(PodcastItem[mpiJsonInfo]);
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
          if (LeftCopy(sLink, 2)=='//') sLink = 'http:'+Trim(sLink);
          if (bDirectLinks)
            sLink = GetMP4LinkHdgo(sLink);
          CreateMediaItem(PodcastItem, sName, sLink, sImg, sGrp, nTime);
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
          if (LeftCopy(sLink, 2)=='//') sLink = 'http:'+Trim(sLink);
          HmsRegExMatch("/(\\d)/", sLink, sVal);
          if      (sVal=="1") sName = "[360p] "+JSON.S["name"];
          else if (sVal=="2") sName = "[480p] "+JSON.S["name"];
          else if (sVal=="3") sName = "[720p] "+JSON.S["name"];
          else if (sVal=="4") sName = "[1080p] "+JSON.S["name"];
          sLink = GetMP4LinkHdgo(sLink, true);
          CreateMediaItem(PodcastItem, sName, sLink, mpThumbnail, "", nTime);
        } while (RE.SearchAgain); else {
          ErrorItem("Video not found!");
        }
        
      } else {
        sSelectedQual = DetectQualityIndex();
        sLink += sSelectedQual;
        if (bDirectLinks)
          sLink = GetMP4LinkHdgo(sLink);
        CreateMediaItem(PodcastItem, sName, sLink, mpThumbnail, "", nTime);
      }
    }    
  } finally { JSON.Free; HDGO.Free; }
}

///////////////////////////////////////////////////////////////////////////////
void CreateInfoLinks() {
  string sID, sImg; int nTime = 210;
  // Создание информационных ссылок
  if (Pos('--infoitems', mpPodcastParameters) > 0) {
    TJsonObject VIDEO = TJsonObject.Create();
    try {
      VIDEO.LoadFromString(PodcastItem[mpiJsonInfo]);
      if (VIDEO.B["kpid" ]) sImg = 'https://st.kp.yandex.net/images/film_iphone/iphone360_'+VIDEO.S["kpid"]+'.jpg';
      if (VIDEO.B["image"]) sImg = 'https://st.kp.yandex.net/images/'+VIDEO.S["image"];
      if (VIDEO.B["trailer_hd" ]) CreateMediaItem(PodcastItem, 'Трейлер HD' , VIDEO.S["trailer_hd" ], sImg, "", nTime);
      if (VIDEO.B["trailer_sd" ]) CreateMediaItem(PodcastItem, 'Трейлер SD' , VIDEO.S["trailer_sd" ], sImg, "", nTime);
      if (!VIDEO.B["trailer_hd"] && !VIDEO.B["trailer_sd"] && VIDEO.B["yt"]) {
        sImg = "https://img.youtube.com/vi/"+VIDEO.S["yt"]+"/sddefault.jpg";
        CreateMediaItem(PodcastItem, 'Трейлер (youtube)', "https://www.youtube.com/watch?v="+VIDEO.S["yt"], sImg, "", nTime);
      }
      InfoItem('Жанр'    , VIDEO.S["genre"   ]);
      if (VIDEO.I["year"]!=0) InfoItem('Год', VIDEO.S["year"]);
      InfoItem('Страна'  , VIDEO.S["country" ]);
      InfoItem('Перевод' , VIDEO.S["translator"]);
      InfoItem('Режиссёр', VIDEO.S["director"]);
      if (VIDEO.B["rating_kp"  ]) InfoItem('Рейтинг КП'  , VIDEO.S["rating_kp"  ]+' ('+VIDEO.S["rating_kp_votes"  ]+')');
      if (VIDEO.B["rating_imdb"]) InfoItem('Рейтинг IMDb', VIDEO.S["rating_imdb"]+' ('+VIDEO.S["rating_imdb_votes"]+')');
      InfoItem('Качество', VIDEO.S["quality"   ]);
    } finally { VIDEO.Free; }
  }
}

///////////////////////////////////////////////////////////////////////////////
string LoadHdgoHtml(string sLink, string &sReferer) {
  string sHtml; HmsRegExMatch("(https?://.*?/)", sLink, sReferer);
  sHtml = HmsDownloadURL(sLink, "Referer: http://hdgo.cc");
  if (HmsRegExMatch('[^\']<iframe[^>]+src="(.*?)"', sHtml, sLink)) {
    if (LeftCopy(sLink, 2)=="//") sLink = "http:"+Trim(sLink);
    HmsRegExMatch("(https?://.*?/)", sLink, sReferer);
    sHtml = HmsDownloadURL(sLink, "Referer: http://hdgo.cc");
  }
  if (!HmsRegExMatch("url:\\s*'(http|//)", sHtml, '') && HmsRegExMatch('<iframe[^>]+src="(.*?)"', sHtml, sLink)) {
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
//                     Г Л А В Н А Я   П Р О Ц Е Д У Р А                     //
{
  if (PodcastItem.IsFolder) {
    CreateHdgoLinks(mpFilePath);
    CreateInfoLinks();
  } else {
    if (HmsFileMediaType(mpFilePath)==mtVideo)
      MediaResourceLink = mpFilePath;
    else if (LeftCopy(mpFilePath, 4)=='-Fav')
      AddRemoveFavorites();
    else if (Pos("youtube", mpFilePath)>0)
      GetLink_Youtube31(mpFilePath);
    else if (LeftCopy(mpFilePath, 4)=='Info')
      VideoPreview();
    else
      MediaResourceLink = GetMP4LinkHdgo(mpFilePath);
  }
}