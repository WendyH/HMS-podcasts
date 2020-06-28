THmsScriptMediaItem PodcastItem = FolderItem; string MediaResourceLink;
///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
string gsUrlBase, gsLongBase, gsHeaders, gsPHPProxy;
TStrings            VARS           = TStringList.Create(); // Хранилище значений полученных переменных
TJsonObject         CONFIG         = TJsonObject.Create(); // Загруженная конфигурация
THmsScriptMediaItem ROOT           = GetRoot(); // Главная папка подкаста
int                 gnItemsAdded   = 0;   // Количество созданных ссылок
TDateTime           gStart         = Now; // Время начала срабатывания скрипта
string              gsAudioPrifile = "Музыка (Визуализация)"; // Профиль по-умолчанию для аудио-файлов
///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

///////////////////////////////////////////////////////////////////////////////
// Установка переменной Podcast: поиск родительской папки, содержащий скрипт
THmsScriptMediaItem GetRoot() {
  ROOT = PodcastItem; // Начиная с текущего элемента, ищется содержащий скрипт
  while ((Trim(ROOT[510])=='') && (ROOT.ItemParent!=nil)) ROOT=ROOT.ItemParent;
  CONFIG.LoadFromString(ROOT[510]);
  if (CONFIG.SaveToString()=='') Error('Ошибка загрузки конфигурации! Возможно, где-то ошибка.');
  return ROOT;
}

//////////////////////////////////////////////////////////////////////////////
// Создание ссылки-ошибки
void Error(string sMsg) {
  THmsScriptMediaItem Item = HmsCreateMediaItem('Err'+IntToStr(gnItemsAdded), PodcastItem.ItemID);
  Item[mpiTitle     ] = sMsg;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/icons/symbol-error.png'; gnItemsAdded++;
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки на видео/аудио
THmsScriptMediaItem Media(string sTitle, string sLink, string sImg='', string sTime='', string sGrp='') {
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, PodcastItem.ItemID, sGrp);
  Item[mpiTitle     ] = sTitle;
  Item[mpiThumbnail ] = sImg;
  Item[mpiCreateDate] = VarToStr(IncTime(gStart,0,-gnItemsAdded,0,0)); gnItemsAdded++;
  Item[mpiTimeLength] = sTime;
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Создание информационной ссылки
THmsScriptMediaItem Info(string sKey, string sVal) {
  sVal = Trim(HmsHtmlToText(sVal)); if (sVal=='') return;
  return Media(sKey+': '+sVal, 'Info'+sKey, 'http://wonky.lostcut.net/vids/info.jpg', '00:00:07.000');
}

///////////////////////////////////////////////////////////////////////////////
// Создание папки
THmsScriptMediaItem AddFolder(string sName, string sLink, string sImg='') {
  THmsScriptMediaItem Item = PodcastItem.AddFolder(sLink);
  Item[mpiTitle          ] = sName;
  Item[mpiThumbnail      ] = sImg;
  Item[mpiCreateDate     ] = DateTimeToStr(IncTime(gStart, 0, -gnItemsAdded, 0, 0)); gnItemsAdded++; // Для обратной сортировки по дате создания
  Item[mpiFolderSortOrder] = "-mpCreateDate";
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Генерация ссылки на видео с информацией
void InfoLink() {
  if (Trim(PodcastItem.ItemParent[mpiComment])=='') { MediaResourceLink=''; return; } 
  string sData = 'text='+HmsHttpEncode(HmsBase64Encode(HmsUtf8Encode(PodcastItem.ItemParent[mpiComment])))+'&urlpic='+HmsPercentEncode(PodcastItem.ItemParent[mpiThumbnail])+'&h='+str(cfgTranscodingScreenHeight)+'&w='+str(cfgTranscodingScreenWidth);
  string sFile = ExtractShortPathName(HmsTranscodingTempDirectory)+'videopreview_';
  string sLink = HmsSendRequest('wonky.lostcut.net', '/videopreview.php', 'POST', 'application/x-www-form-urlencoded;', '', sData, 80); if (LeftCopy(sLink, 4)!='http') return;
  HmsDownloadURLToFile(sLink, sFile);
  for (int n=0; n<Min(60, HmsTimeConvert(mpTimeLength)); n++) CopyFile(sFile, sFile+Format('%.3d.jpg', [n]), false);
  string sMP3 = ExtractShortPathName(HmsTempDirectory)+'\\silent.mp3';
  try {
    if (!FileExists(sMP3)) HmsDownloadURLToFile('http://wonky.lostcut.net/mp3/silent.mp3', sMP3);
    sMP3 = '-i "'+sMP3+'"';
  } except { sMP3=''; }
  MediaResourceLink = Format('%s -f image2 -r 1 -i "%s" -c:v libx264 -pix_fmt yuv420p ', [sMP3, sFile+'%03d.jpg']);
}

///////////////////////////////////////////////////////////////////////////////
// Универсальное определение длительности из полученного текста
string GetTimeLength(string sDura) {
  int nH, nM, nS, nSecs, nMins; string sH, sM, sS; if (Trim(sDura)=='') return '';
  if      (HmsRegExMatch3('(\\d*):(\\d*):(\\d\\d?)', sDura, sH, sM, sS)) { nH=StrToInt(sH); nM=StrToInt(sM); nS=StrToInt(sS); }
  else if (HmsRegExMatch2(       '(\\d*):(\\d\\d?)', sDura,     sM, sS)) { nM=StrToInt(sM); nS=StrToInt(sS); nH=Trunc(nM/60); nM=nM-(nH*60); }
  else if (HmsRegExMatch2('(\\d+)m(\\d+)s'         , sDura,     sM, sS)) { nM=StrToInt(sM); nS=StrToInt(sS); nH=Trunc(nM/60); nM=nM-(nH*60); }
  else if (HmsRegExMatch ('(\\d+)\\s*(мин|min)'    , sDura,     sM    )) { nM=StrToInt(sM); nH=Trunc(nM/60); nM=nMins-(nH*60); nS=0; }
  else if (HmsRegExMatch ('(\\d+)'                 , sDura,         sS)) { nS=StrToInt(sS); nH=Trunc(nSecs/60/60); nM=Trunc(nSecs/60)-(nH*60); nS=nSecs-(nM*60)-(nH*60*60); }
  return format("%.2d:%.2d:%.2d.000", [nH, nM, nS]);
}

///////////////////////////////////////////////////////////////////////////////
// Установка глобальной переменной gsHeaders
string GetHeaders(TJsonObject HEADERS) {
  string sHeaders = '';
  for (int i=0; i<HEADERS.Count; i++) {
    string sName = HEADERS.Names(i);
    string sVal  = HEADERS.S[sName], sOld;
    sHeaders += sName+': '+sVal+'\r\n';
  }
  return sHeaders;
}

///////////////////////////////////////////////////////////////////////////////
string HttpRequest(string sUrl, TJsonObject CONF, bool bWithHeaders=false) {
  string sHtml, sServ, sPage, sReferer, sRet, sPost, sVal, sMethod; int nPort=80, nFlags=0x10;
  if (gsPHPProxy!='') sUrl = gsPHPProxy+'?'+HmsPercentDecode(sUrl);
  sMethod = CONF.S['Method']; if (sMethod=='') sMethod='GET';
  HmsRegExMatch2('//([^/]+)(/.*)', sUrl, sServ, sPage);
  if (LeftCopy(sUrl, 5)=='https') nPort = 443;
  if (HmsRegExMatch(':(\\d+)//', sUrl, sVal)) nPort = StrToInt(sVal);
  if (CONF.B['NoCache'   ]) nFlags = StrToInt('$80000000');
  if (CONF.B['NoRedirect']) nFlags += 0x00200000;
  if (CONF.B['PostData'  ]) sPost  = CONF.S['PostData'];
  if (HmsRegExMatch('<\\w+>', sPost, '')) { GetVars(CONF, sHtml); sPost = ReplaceVars(sPost); }
  sReferer = sUrl + "\r\n" + gsHeaders;
  if (sMethod=='POST') HmsRegExMatch('\\?([^#]+)', sUrl, sPost);
  sHtml = HmsSendRequestEx(sServ, sPage, sMethod, 'application/x-www-form-urlencoded; Charset=UTF-8', sReferer, sPost, nPort, nFlags, sRet, true);
  if (bWithHeaders) return sRet + sHtml; else return sHtml;
}

///////////////////////////////////////////////////////////////////////////////
// Вход на сайт
bool Login(TJsonObject CONF, string &sHtml) {
  string sData, sPost, sUrl = CONF.S['Url'];
  if ((mpPodcastAuthorizationPassword=='') || (mpPodcastAuthorizationUserName=='')) { Error('Для сайта '+gsUrlBase+' необходимо ввести логин и пароль.'); return false; }
  if (HmsRegExMatch('<\\w+>', sUrl, '')) { sUrl = ReplaceVars(sUrl); }
  sData = HttpRequest(sUrl, CONF, true);
  return HmsRegExMatch(CONF.S['Success'], sData, '');
}

///////////////////////////////////////////////////////////////////////////////
// Замена указанных переменных типа <NAME> на их значения
string ReplaceVars(string sVal) {
  if (!HmsRegExMatch('<\\w+>', sVal, '')) return sVal;
  for (int i=0; i < VARS.Count; i++) sVal = ReplaceStr(sVal, '<'+VARS.Names[i]+'>', VARS.Values[VARS.Names[i]]);
  sVal = ReplaceStr(sVal, '<LOGIN>', HmsPercentEncode(mpPodcastAuthorizationUserName));
  sVal = ReplaceStr(sVal, '<PASSW>', HmsPercentEncode(mpPodcastAuthorizationPassword));
  return sVal;
}

///////////////////////////////////////////////////////////////////////////////
// Получение значений из html 
void GetVars(TJsonObject CONF, string &sHtml) {
  string sName, sVal;
  for (int i=0; i<CONF.Count; i++) {
    sName = CONF.Names[i]; sVal = '';
    HmsRegExMatch(CONF.S[sName], sHtml, sVal, 1, PCRE_SINGLELINE || PCRE_CASELESS);
    sVal = Trim(HmsHtmlToText(sVal)); if (sVal=='') continue;
    if (HmsRegExMatch('^(Link|Image|Url)$', sName, '')) {
      // Восстанавливаем ссылку из относительной в полную (если нужно)
      if      (LeftCopy(sVal, 2)=='//'  ) sVal = 'http:'+ Trim(sVal);
      else if (LeftCopy(sVal, 1)=='/'   ) sVal = HmsExpandLink(sVal, gsUrlBase );
      else if (LeftCopy(sVal, 2)=='./'  ) sVal = gsLongBase + Copy(sVal, 3, 999);
      else if (LeftCopy(sVal, 4)!='http') sVal = HmsExpandLink(sVal, gsLongBase);
    }
    else if (sName=='Duration') sVal = GetTimeLength  (sVal);
    VARS.Values[sName] = sVal;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Получение имени файла для TorrServer (https://github.com/YouROK/TorrServer/blob/master/src/server/utils/Utils.go)
string CleanFName(string sName) {
  string sRep=' \\!*\'();:@&=+$,/?#[]~"';
  for (int i=1; i<=Length(sRep); i++) sName = ReplaceStr(sName, sRep[i], '_'); 
  sName = ReplaceStr(sName, '__', '_');
  return sName;
}

///////////////////////////////////////////////////////////////////////////////
// Добавление torrent-файла в TorrServer
string AddTorrentFile2TorrServer(string sFile) {
  string sServ, sData, sServBase, sVal, sPost, sHash, sTorrPath, fID; int nPort=8090;
  if (!HmsRegExMatch("--torrserver=([^\\s]+)", mpPodcastParameters, sServ) || !FileExists(sFile)) return '';
  sData = HmsStringFromFile(sFile); sServBase = 'http://'+sServ; fID = 'TheCoolFileForTorrServer25LMdsd0as';
  if (HmsRegExMatch2('^(.*?):(\\d+)', sServ, sServ, sVal)) nPort = StrToInt(sVal);
  sPost = '------'+fID+'\r\nContent-Disposition: form-data; name="DontSave"\r\n\r\nTrue\r\n'+
          '------'+fID+'\r\nContent-Disposition: form-data; name="file-0"; filename="'+sFile+'"\r\nContent-Type: application/x-bittorrent\r\n\r\n'+sData+'\r\n'+
          '------'+fID+'--\r\n';
  sHash = HmsSendRequest(sServ, '/torrent/upload', 'POST', 'multipart/form-data; boundary=----'+fID, '', sPost, nPort);
  HmsRegExMatch('"(.*?)"', sHash, sHash);
  if (sHash!='') sTorrPath = sServBase+'/torrent/view/'+sHash;
  return sTorrPath;
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылок файлов торрента
void CreateLinksFromTorrentFile(string sFile) {
  string sData, sName, sLink, sVal, sTorrPath; int nSecTotal, i, n; THmsScriptMediaItem Item; bool bExistVideo, bExistAudio;   
  
  // Если в настройках указан адрес сервера torrserver, то пробуем работать через него
  if (Pos("--torrserver=", mpPodcastParameters)>0) sTorrPath = AddTorrentFile2TorrServer(sFile);
  
  TTorrentFile TorrentFile = TTorrentFile.Create();  
  try {
    TorrentFile.LoadFromFile(sFile);
    
    if (TorrentFile.MultiFile) {
      PodcastItem[mpiFolderSortOrder] = "mpTitle";
      for (i=0; i<TorrentFile.Count; i++) {
        TTorrentSubFile TorrentSubFile = TorrentFile.Files[i];
        if (sTorrPath!='') {
          sLink = sTorrPath+'/'+CleanFName(TorrentFile.Name+'/'+TorrentSubFile.Path+TorrentSubFile.Name);
        } else {
          sLink = Format('torrent:%s?index=%d', [sFile, i]);
        }
        sName = TorrentSubFile.Name;        
        
        if (HmsFileMediaType(sName) == mtVideo) {
          bExistVideo = true;
          Item = Media(sName, sLink, '', mpTimeLength, TorrentSubFile.Path);
          Item[mpiFileSize] = TorrentSubFile.Length;
          Item[mpiComment ] = sFile;
          Item.CopyProperties(PodcastItem, [mpiThumbnail, mpiTimeLength, mpiYear, mpiGenre]);
          
        } else if (HmsFileMediaType(sName) == mtAudio) {
          bExistAudio = true;
          Item = Media(sName, sLink, '', mpTimeLength, TorrentSubFile.Path);
          Item[mpiFileSize] = TorrentSubFile.Length;
          Item[mpiComment ] = sFile;
          Item.CopyProperties(PodcastItem, [mpiThumbnail, mpiTimeLength, mpiYear, mpiGenre, mpiAuthor, mpiAlbum, mpiAlbumArtist]);
          //Item[mpiTranscodingProfile] = gsAudioPrifile;
          if (Trim(Item[mpiThumbnail])=='') Item[mpiThumbnail] = 'http://wonky.lostcut.net/icons/3Dsound.jpg';
          nSecTotal = HmsTimeConvert(Item[mpiTimeLength]);
          if (nSecTotal>0) Item[mpiTimeLength] = HmsTimeFormat(Int(nSecTotal * TorrentSubFile.Length / TorrentFile.Length));
        }      
      }
      
    } else {     
      if (sTorrPath!='') {
        sLink = sTorrPath+'/'+CleanFName(TorrentFile.Name);
      } else {
        sLink = Format('torrent:%s?index=%d', [sFile, 0]);
      }
      sName = TorrentFile.Name;
      if (sName=="") sName = PodcastItem[4]+".avi";        
      if (HmsFileMediaType(sName) == mtVideo) {
        bExistVideo = true;
        Item = Media(sName, sLink);
        Item[mpiFileSize] = TorrentFile.Length;
        Item[mpiComment ] = sFile;
        Item.CopyProperties(PodcastItem, [mpiThumbnail, mpiTimeLength, mpiYear, mpiGenre]);
        
      } else if (HmsFileMediaType(sName) == mtAudio) {
        bExistAudio = true;
        Item = Media(sName, sLink);
        Item[mpiFileSize] = TorrentFile.Length;
        Item[mpiComment ] = sFile;
        Item.CopyProperties(PodcastItem, [mpiThumbnail, mpiTimeLength, mpiYear, mpiGenre, mpiAuthor, mpiAlbum, mpiAlbumArtist]);
        if (Trim(Item[mpiThumbnail])=='') Item[mpiThumbnail] = 'http://wonky.lostcut.net/icons/3Dsound.jpg';
        
      }      
      
    }
  } finally { TorrentFile.Free; }
  
  if (!bExistVideo && !bExistAudio) Error('Нет видео или аудио файлов.');
}

///////////////////////////////////////////////////////////////////////////////
// Поиск ссылки на скачивание торрент-файла и получение информационных о нём
void DownloadTorrent(string &sHtml, TJsonObject RULE) {
  string sData, sUrl, sFile, sID, sMsg; int i, n; TJsonObject JSON;
  
  // Поиск ссылки для скачивания
  HmsRegExMatch(RULE.S['Url'], sHtml, sUrl);
  if (sUrl=='') {
    Error('Нет ссылки на скачивание torrent-файла.');
    if (RULE.B['Error'] && HmsRegExMatch(RULE.S['Error'], sHtml, sMsg)) Error(sMsg);
    return;
  }
  
  // Восстанавливаем ссылку из относительной в полную (если нужно)
  if      (LeftCopy(sUrl, 2)=='//'  ) sUrl = 'http:'+ Trim(sUrl);
  else if (LeftCopy(sUrl, 1)=='/'   ) sUrl = HmsExpandLink(sUrl, gsUrlBase );
  else if (LeftCopy(sUrl, 4)!='http') sUrl = HmsExpandLink(sUrl, gsLongBase);
  
  GetVars(RULE, sHtml); // Получение значений переменных из загруженной html-страницы
  
  sData = HttpRequest(sUrl, RULE); // Скачиваем torrent-файл по указанному правилу
  
  if (Pos('8:announce', sData)<1) {
    Error('Ошибка получения torrent файла '+sUrl);
    if (RULE.B['Error'] && HmsRegExMatch(RULE.S['Error'], sHtml, sMsg)) Error(sMsg);
    return;
  }
  
  if (VARS.Values['Duration']!='') PodcastItem[mpiTimeLength] = VARS.Values['Duration'];
  if (VARS.Values['Image'   ]!='') PodcastItem[mpiThumbnail ] = VARS.Values['Image'   ];
  
  // Сохраняем торрент в файл
  sFile = IncludeTrailingBackslash(HmsTranscodingTempDirectory+'Torrents')+PodcastItem.ItemID+'.torrent';
  HmsStringToFile(sData, sFile);
  
  // Создание ссылок на файлы внутри торрент-файла
  CreateLinksFromTorrentFile(sFile);
  
  // Ищем ID кинопоиска или IMDb, если нашли - запрашиваем рейтинг
  string sRatingKP, sRatingIM, sRating, sVotes;
  if (HmsRegExMatch('kinopoisk.ru/(?:rating|film)/(\\d+)', sHtml, sID)) {
    //Media('Трейлер с КиноПоиск', 'trailerKP='+sID, 'https://st.kp.yandex.net/images/film_iphone/iphone360_'+sID+'.jpg', '00:03:30.000');
    sData = HmsDownloadURL('https://rating.kinopoisk.ru/'+sID+'.xml');
    if (HmsRegExMatch2(  'kp_rating[^>]+num_vote="(\\d+)">(.*?)<', sData, sVotes, sRating)) sRatingKP = sRating+' ('+sVotes+')';
    if (HmsRegExMatch2('imdb_rating[^>]+num_vote="(\\d+)">(.*?)<', sData, sVotes, sRating)) sRatingIM = sRating+' ('+sVotes+')';
    //sData = HmsSendRequest('ahoy.yohoho.online', '/', 'POST', 'application/x-www-form-urlencoded', 'http://www.hdkinoteatr.com/\r\n', 'kinopoisk='+sID, 443);
    //sData = HmsDownloadURL('http://api.lostcut.net/hdkinoteatr/videos?kpid='+sID);
  }
  
  if (HmsRegExMatch('imdb[\\.com]*/\\w+/[t]*(\\d+)', sHtml, sID)) {
    sData = HmsDownloadURL('http://www.omdbapi.com/?apikey=30ce34f1&i=tt'+sID);
    JSON = TJsonObject.Create();
    JSON.LoadFromString(HmsUtf8Decode(sData));
    Media('Трейлер с IMDb', 'trailerIM='+sID, JSON.S['Poster'], '00:03:00.000');
    if (JSON.B['imdbRating']) {
      Info('Режиссёр'    , JSON.S['Director']);
      Info('Актёры'      , JSON.S['Actors'  ]);
      Info('Релиз'       , JSON.S['Released']);
      Info('Страна'      , JSON.S['Country' ]);
      sRatingIM = JSON.S['imdbRating']+' ('+JSON.S['imdbVotes']+')';
    }
  }
  
  Info('Рейтинг IMDb'     , sRatingIM);
  Info('Рейтинг КиноПоиск', sRatingKP);
  
  // Создание информационных ссылок
  Info('Раздают' , VARS.Values['Seeders'  ]);
  Info('Качают'  , VARS.Values['Leechers' ]);
  Info('Размер'  , VARS.Values['Size'     ]);
  Info('Добавлен', VARS.Values['Date'     ]);
  Info('Год'     , VARS.Values['Year'     ]);
  Info('Жанр'    , VARS.Values['Genre'    ]);
  Info('Cубтитры', VARS.Values['Subs'     ]);
  Info('Качество', VARS.Values['Quality'  ]);
  Info('Скачало' , VARS.Values['Downloads']);
  Info('Автор'   , VARS.Values['Author'   ]);
  Info('Альбом'  , VARS.Values['Album'    ]);

  PodcastItem[mpiComment] = VARS.Values['Text']; // Запоминаем для отображения в Info ссылках
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылок
void CreateLinks() {
  string sHtml, sData, sVal, sLink, sName, sImg, sGrp; int i, n; THmsScriptMediaItem Item; bool bNeedLogin;
  // Загружаем страницу и проверяем, нужно ли нам залогиниться
  sHtml = HmsDownloadURL(mpFilePath, gsHeaders);
  if (HmsRegExMatch('charset="?utf-?8', sHtml, '', 1, PCRE_CASELESS))  sHtml = HmsUtf8Decode(sHtml);
  bNeedLogin = CONFIG.B['Login']; if (CONFIG.B['LoginCondition']) bNeedLogin = HmsRegExMatch(CONFIG.S['LoginCondition'], sHtml, '');
  if (bNeedLogin) { 
    if (!Login(CONFIG['Login'], sHtml)) return; 
    sHtml = HmsDownloadURL(mpFilePath, gsHeaders); // После логина - перезагружаем страницу заново
    if (HmsRegExMatch('charset="?utf-?8', sHtml, '', 1, PCRE_CASELESS)) sHtml = HmsUtf8Decode(sHtml);
  }
  // Перебираем все условия
  TJsonArray CONDITIONS = CONFIG['Conditions'].AsArray;
  if (CONDITIONS != nil) {
    for (i=0; i < CONDITIONS.Length; i++) {
      TJsonObject COND = CONDITIONS[i];
      if (COND.B['ConditionLink'] && !HmsRegExMatch(COND.S['ConditionLink'], mpFilePath, '')) continue; // Условие на совпадение в ссылке
      if (COND.B['ConditionText'] && !HmsRegExMatch(COND.S['ConditionText'], sHtml     , '')) continue; // Условие на совпадение в тексте
      if (COND.B['Rule' ]) { DownloadTorrent(sHtml, CONFIG[COND.S['Rule']]); return; } // Если указано правило - создаём ссылки по этому правилу
      if (COND.B['Block']) {                                                           // Если указан Block - ищем эти блоки и создаём папки
        TRegExpr RegEx = TRegExpr.Create(COND.S['Block'], PCRE_SINGLELINE);
        if (RegEx.Search(sHtml)) do {
          sName=''; sLink=''; sImg=''; sGrp='';
          HmsRegExMatch(COND.S['Title'], RegEx.Match(0), sName); sName = HmsHtmlToText(sName);
          HmsRegExMatch(COND.S['Link' ], RegEx.Match(0), sLink); if (sLink=='') continue;
          if (Length(sName) > 127 ) sName = LeftCopy(sName, 127); // На некоторых телевизорах крашится, если имя слишком длинное
          // Восстанавливаем ссылку из относительной в полную (если нужно)
          if      (LeftCopy(sLink, 2)=='//'  ) sLink = 'http:'+ Trim(sLink);
          else if (LeftCopy(sLink, 1)=='/'   ) sLink = HmsExpandLink(sLink, gsUrlBase );
          else if (LeftCopy(sLink, 2)=='./'  ) sLink = gsLongBase + Copy(sLink, 3, 999);
          else if (LeftCopy(sLink, 4)!='http') sLink = HmsExpandLink(sLink, gsLongBase);
          AddFolder(sName, HmsHtmlDecode(sLink));
        } while (RegEx.SearchAgain());
      }
      if ((COND.B['NextPage']) && HmsRegExMatch(COND.S['NextPage'], sHtml, sLink)) { // Поиск ссылки на следующую страницу
        // Восстанавливаем ссылку из относительной в полную (если нужно)
        if      (LeftCopy(sLink, 2)=='//'  ) sLink = 'http:'+ Trim(sLink);
        else if (LeftCopy(sLink, 1)=='/'   ) sLink = HmsExpandLink(sLink, gsUrlBase );
        else if (LeftCopy(sLink, 2)=='./'  ) sLink = gsLongBase + Copy(sLink, 3, 999);
        else if (LeftCopy(sLink, 4)!='http') sLink = HmsExpandLink(sLink, gsLongBase);
        if (mpPartNo<3) { Item=AddFolder('След. страница', sLink); Item[mpiPartNo] = mpPartNo+1; }
      }
    }
  }
  return;
}

///////////////////////////////////////////////////////////////////////////////
// Получение ссылки на трейлер по ID KinoPoisk
void GetTrailerLinkKP(string sID) {
  string sHtml = HmsDownloadURL('https://www.kinopoisk.ru/film/'+sID+'/video/');
  if (HmsRegExMatch('/film/\\d+/video/(\\d+)/', sHtml, sID)) {
    sHtml = HmsDownloadURL('https://widgets.kinopoisk.ru/discovery/trailer/'+sID+'?onlyPlayer=1');
    HmsRegExMatch('"streamUrl":"(.*?)"', sHtml, MediaResourceLink);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Получение ссылки на трейлер по ID IMDb
void GetTrailerLinkIM(string sID) {
  string sHtml = HmsDownloadURL('https://www.imdb.com/title/tt'+sID);
  if (HmsRegExMatch('/video/imdb/(\\w+)', sHtml, sID)) {
    sHtml = HmsDownloadURL('https://www.imdb.com/video/'+sID);
    HmsRegExMatch('"url\\\\":\\\\"([^\\\\"]+)', sHtml, MediaResourceLink);
    if (HmsRegExMatch('"video/mp4\\\\",\\\\"url\\\\":\\\\"([^\\\\"]+)', sHtml, MediaResourceLink))
    {
      //PodcastItem[mpiMimeType  ] = 'video/mp4';
      //PodcastItem[mpiDirectLink] = 1;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Создание папок из локальных torrent-файлов
void CreateLinksFromLocalFolder() {
  string sData, sName, sLink, sFile, sServ, sTorrServerPath; int i; THmsScriptMediaItem Item;
  
  if (FileExists(mpFilePath) && (LowerCase(ExtractFileExt(mpFilePath))=='.torrent')) {
    sData = HmsStringFromFile(mpFilePath);
    if (Pos('8:announce', sData)<1) {
      Error('Неверный torrent файл');
      return;
    }
    CreateLinksFromTorrentFile(mpFilePath);
    return;
  }  
  
  TStrings FILES   = TStringList.Create();
  TStrings FOLDERS = TStringList.Create();
  TTorrentFile TorrentFile = TTorrentFile.Create();  
  try {
    
    HmsGetDirectoryList(mpFilePath, FOLDERS); 
    HmsGetFileList(mpFilePath, FILES, '*.torrent'); 
    
    if ((FOLDERS.Count+FILES.Count)==0) {
      Error('Каталог пуст или указан неправильно');
      return;
    }
    
    for (i=0; i<FOLDERS.Count; i++) {
      sLink = FOLDERS.Strings[i];
      sName = ExtractFileName(sLink);
      AddFolder(sName, ReplaceStr(sLink, '\\', '/'));
    }
    for (i=0; i<FILES.Count; i++) {
      sLink = FILES.Strings[i];
      sName = ExtractFileName(sLink);
      TorrentFile.LoadFromFile(sLink);
      if (Trim(TorrentFile.Name)!='') sName = TorrentFile.Name;
      AddFolder(sName, ReplaceStr(sLink, '\\', '/'));
    }
    
  } finally {FILES.Free; FOLDERS.Free; TorrentFile.Free; }
}

///////////////////////////////////////////////////////////////////////////////
// Поиск установленного логина и пароля в папке соответствующего сайта
void SetPassword4Site(string sDomen) {
  for (int i=0; i < ROOT.ChildCount; i++) {
    THmsScriptMediaItem Item = ROOT.ChildItems[i];
    if (Pos(sDomen, Item[mpiFilePath])>0) {
      mpPodcastAuthorizationUserName = Item[590];
      mpPodcastAuthorizationPassword = Item[591]; // Он тут зашифрован, поэтому извращаемся далее
      THmsScriptMediaItem TmpLink = HmsCreateMediaItem('-getpass', Item.ItemID);
      TmpLink.RetrieveProperties();
      mpPodcastAuthorizationPassword = TmpLink[mpiFilePath];
      TmpLink.Delete();
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Поиск названия на всех доступных сайтах
void SearchTitle() {
  string sHtml, sLink, sName, sUrl, sDomen; TJsonObject OBJ, CONF; TJsonArray SEARCHES=CONFIG['Search'].AsArray;
  for (int i=0; i < SEARCHES.Length; i++) {
    OBJ   = SEARCHES[i]; if (!OBJ.B['Enable']) continue;
    CONF  = CONFIG[OBJ.S['Site']];
    sName = mpTitle; if (OBJ.B['Utf8']) sName = HmsUtf8Encode(sName);   
    sUrl  = ReplaceStr(OBJ.S['Url'], '<TITLE>', HmsPercentEncode(sName));
    HmsRegExMatch2('^((.*?//[^/]+).*/)', sUrl, gsLongBase, gsUrlBase);
    gsPHPProxy = OBJ.S['PHProxy'];
    if (gsPHPProxy!='') sUrl = gsPHPProxy+'?'+HmsPercentDecode(HmsExpandLink(sUrl, gsUrlBase));
    if (CONFIG.B['Headers']) gsHeaders += GetHeaders(CONFIG['Headers']);
    // Загружаем страницу и проверяем, нужно ли нам залогиниться
    sHtml = HmsDownloadURL(sUrl, gsHeaders);
    if (HmsRegExMatch('charset="?utf-?8', sHtml, '', 1, PCRE_CASELESS))   sHtml = HmsUtf8Decode(sHtml);
    bool bNeedLogin = CONF.B['Login']; if (CONF.B['LoginCondition']) bNeedLogin = HmsRegExMatch(CONF.S['LoginCondition'], sHtml, '');
    if (bNeedLogin) { 
      // А вот тут нужно найти и переопределить логин и пароль для текущего сайта
      HmsRegExMatch('(//[^/:]+)', sUrl, sDomen);
      SetPassword4Site(sDomen); // Переустановим текущие значения mpPodcastAuthorizationUserName и mpPodcastAuthorizationPassword
      if (!Login(CONF['Login'], sHtml)) return; 
      sHtml = HmsDownloadURL(sUrl, gsHeaders); // После логина - перезагружаем страницу заново
      if (HmsRegExMatch('charset="?utf-?8', sHtml, '', 1, PCRE_CASELESS)) sHtml = HmsUtf8Decode(sHtml);
    }
    // Производим непосредственно поиск нужных блоков со ссылками и названием
    TRegExpr RegEx = TRegExpr.Create(OBJ.S['Block'], PCRE_SINGLELINE);
    if (RegEx.Search(sHtml)) do {
      if (!HmsRegExMatch(OBJ.S['Title'], RegEx.Match(0), sName)) continue;
      if (!HmsRegExMatch(OBJ.S['Link' ], RegEx.Match(0), sLink)) continue;
      sName = HmsHtmlToText(sName); if (Length(sName)>127) sName = LeftCopy(sName, 127);
      HmsRegExMatch('^\\.(/.*)', sLink, sLink);
      // Восстанавливаем ссылку из относительной в полную (если нужно)
      if      (LeftCopy(sLink, 2)=='//'  ) sLink = 'http:'+ Trim(sLink);
      else if (LeftCopy(sLink, 1)=='/'   ) sLink = HmsExpandLink(sLink, gsUrlBase );
      else if (LeftCopy(sLink, 4)!='http') sLink = HmsExpandLink(sLink, gsLongBase);
      AddFolder(sName,  HmsHtmlDecode(sLink));
    } while (RegEx.SearchAgain());
  }
}

///////////////////////////////////////////////////////////////////////////////
void CreateLinksFromTorrServer() {
  string sServ, sData, sServBase, sVal, sPost, sHash, sTorrPath, fID; int i, n, nPort=8090; TJsonObject JSON=TJsonObject.Create(), VIDEO, FILE; TJsonArray LIST, FILES;
  if (!HmsRegExMatch("--torrserver=([^\\s]+)", mpPodcastParameters, sServ)) return;
  sServBase = 'http://'+sServ;
  if (HmsRegExMatch2('^(.*?):(\\d+)', sServ, sServ, sVal)) nPort = StrToInt(sVal);
  sData = HmsUtf8Decode(HmsSendRequest(sServ, '/torrent/list', 'POST', 'application/x-www-form-urlencoded;' , '', sPost, nPort));
  JSON.LoadFromString(sData); LIST = JSON.AsArray; if (LIST==nil) return;
  for (i = 0; i < LIST.Length; i++) {
    VIDEO = LIST[i];
    if (VIDEO.B['Files']) {
      FILES = VIDEO['Files'].AsArray;
      for (n=0; n<FILES.Length; n++) {
        FILE = FILES[n];
        Variant Item = Media(FILE.S['Name'], sServBase+FILE.S['Link'], '', '', VIDEO.S['Name']);
        Item[mpiFileSize] = FILE.I['Size'];
      }
    } else {
      Media(VIDEO.S['Name'], sServBase+VIDEO.S['Playlist']);
    }
  }
  JSON.Free;
}

///////////////////////////////////////////////////////////////////////////////
// Проверка и обновление скриптов подкаста
void CheckPodcastUpdate() {
  string sData, sName, sLang, sExt, sMsg; int i, mpiTimestamp=100602, mpiSHA, mpiScript; TJsonObject JSON, JFILE; TJsonArray JARRAY; bool bChanges=false;

  THmsScriptMediaItem Podcast = PodcastItem; // Начиная с текущего элемента, ищется содержащий скрипт
  while ((Trim(Podcast[mpiFilePath])!='-RootTorrentRover') && (Podcast.ItemParent!=nil)) Podcast=Podcast.ItemParent;
  
  // Если после последней проверки прошло меньше получаса - валим
  if ((Trim(Podcast[550])=='') || (DateTimeToTimeStamp1970(Now, false)-StrToIntDef(Podcast[mpiTimestamp], 0) < 14400)) return; // раз в 4 часа
  Podcast[mpiTimestamp] = DateTimeToTimeStamp1970(Now, false); // Запоминаем время проверки
  sData = HmsDownloadURL('https://api.github.com/repos/WendyH/HMS-podcasts/contents/TorrentRover', "Accept-Encoding: gzip, deflate", true);
  JSON  = TJsonObject.Create();
  try {
    JSON.LoadFromString(sData);
    JARRAY = JSON.AsArray(); if (JARRAY==nil) return;
    for (i=0; i<JARRAY.Length; i++) {     // Обходим в цикле все файлы в папке github
      JFILE = JARRAY[i]; if(JFILE.S['type']!='file') continue;
      sName = ChangeFileExt(JFILE.S['name'], ''); sExt = ExtractFileExt(JFILE.S['name']);
      switch (sExt) { case'.cpp':sLang='C++Script'; case'.pas':sLang='PascalScript'; case'.vb':sLang='BasicScript'; case'.js':sLang='JScript'; default:sLang=''; } // Определяем язык по расширению файла
      if      (sName=='Alt1') { mpiSHA=100701; mpiScript=571; sMsg='Требуется запуск "Создать ленты подкастов"'; } // Это скрипт создания подкаст-лент  (Alt+1)
      else if (sName=='Alt2') { mpiSHA=100702; mpiScript=530; sMsg='Требуется обновить раздел заново';           } // Это скрипт чтения списка ресурсов (Alt+2)
      else if (sName=='Alt4') { mpiSHA=100704; mpiScript=550; sMsg=''; }                                           // Это скрипт получения ссылки на ресурс  (Alt+4)
      else if ((sName=='Config') && (sLang=='JScript')) { mpiSHA=100703; mpiScript=510; sMsg='Требуется обновить раздел заново'; } // Это скрипт чтения дополнительных в RSS (Alt+3)
      else continue;                      // Если файл не определён - пропускаем
      if (Podcast[mpiSHA]!=JFILE.S['sha']) { // Проверяем, требуется ли обновлять скрипт?
        sData = HmsDownloadURL(JFILE.S['download_url'], "Accept-Encoding: gzip, deflate", true); // Загружаем скрипт
        sData = ReplaceStr(sData, 'п»ї', ''); // Remove BOM
        if (sData=='') continue;                                                  // Если не получилось загрузить, пропускаем
        Podcast[mpiScript+0] = HmsUtf8Decode(ReplaceStr(sData, '\xEF\xBB\xBF', '')); // Скрипт из unicode и убираем BOM
        Podcast[mpiScript+1] = sLang;                                                // Язык скрипта
        Podcast[mpiSHA     ] = JFILE.S['sha']; bChanges = true;                      // Запоминаем значение SHA скрипта
        if (sName=='Alt2') { // В нашем случае скрипт Alt2 от Alt4 ничем не отличается кроме первой закомментированной строки
          Podcast[550] = '//'+Podcast[mpiScript+0];  // Alt4 скрипт
          Podcast[551] = sLang;
        }
        HmsLogMessage(1, Podcast[mpiTitle]+": Обновлён скрипт подкаста "+sName);     // Сообщаем об обновлении в журнал
        if (sMsg!='') PodcastItem.AddFolder(' !'+sMsg+'!');                       // Выводим сообщение как папку
        
      }
    } 
  } finally { JSON.Free; if (bChanges) HmsDatabaseAutoSave(true); }
} //Вызов в главной процедуре: if (Pos('--nocheckupdates' , mpPodcastParameters)<1) CheckPodcastUpdate();

///////////////////////////////////////////////////////////////////////////////
//                      Г Л А В Н А Я   П Р О Ц Е Д У Р А                    //
// ----------------------------------------------------------------------------
{
  string sDomen, sID, sServ, sHash, sVal, sRet, sData; int nPort=8090; 
  
  if (PodcastItem.IsFolder) {
    PodcastItem.DeleteChildItems(); // Удаление существующих ссылок
    HmsRegExMatch('--muzprofile="(.*?)"', mpPodcastParameters, gsAudioPrifile);
    
    if (Pos('--nocheckupdates' , mpPodcastParameters)<1) CheckPodcastUpdate();
    
    Variant bDirExists = DirectoryExists(mpFilePath); // тип Variant - это баг HMS 2.25
    if ((LeftCopy(mpFilePath, 1)!='/') && (FileExists(mpFilePath) || bDirExists)) { CreateLinksFromLocalFolder(); return; }      
    if (mpFilePath=='-TorrServer') { CreateLinksFromTorrServer(); return; }
    
    // Поиск ссылки на домен (начинающийся на http), если в текущем элементе не полная ссылка
    THmsScriptMediaItem Folder = PodcastItem; // Начиная с текущего элемента, ищется ссылка на домен (вверх по родительским папкам)
    while ((LeftCopy(Folder[mpiFilePath], 4)!='http') && (Folder.ItemParent!=nil) && (Folder[mpiFilePath]!='-SearchFolder')) Folder=Folder.ItemParent;
    
    if ((PodcastItem.ItemClassID==53) && (Folder[mpiFilePath]=='-SearchFolder')) {
      // Если обновляемый подкаст находится в папке "Поиск", то запускаем процесс поиска 
      SearchTitle();

    } else {
      // Иначе просто пытаемся создать ссылки (папки)
      HmsRegExMatch('^(.*?//[^/]+)', Folder[mpiFilePath], gsUrlBase); // Получаем gsUrlBase
      mpFilePath =   HmsExpandLink(mpFilePath, gsUrlBase );           // Если ссылка относительная, делаем из неё полную
      HmsRegExMatch('^(.*?//.*/)', mpFilePath, gsLongBase);           // Получаем gsLongBase (до последнего слеша)
      
      // Пытаемся получить конфиг для данного домена
      HmsRegExMatch('//([^/]+)', mpFilePath, sDomen); CONFIG = CONFIG[sDomen];
      if (CONFIG == nil) { Error('Для домена '+sDomen+' нет правил. Навигация невозможна.'); return; }
      
      gsPHPProxy = CONFIG.S['PHProxy'];
      if (gsPHPProxy!='') mpFilePath = gsPHPProxy+'?'+HmsPercentDecode(HmsExpandLink(mpFilePath, gsUrlBase));
      if (CONFIG.B['Headers']) gsHeaders += GetHeaders(CONFIG['Headers']);
      
      CreateLinks(); // Если всё хорошо, запускаем процесс поиска и создания ссылок
    }
    
  } else {
    if (mpFilePath=='-getpass') { PodcastItem[mpiFilePath] = mpPodcastAuthorizationPassword; return; } // For login in search
    if (HmsRegExMatch('trailerKP=(\\d+)', mpFilePath, sID)) GetTrailerLinkKP(sID);
    if (HmsRegExMatch('trailerIM=(\\d+)', mpFilePath, sID)) GetTrailerLinkIM(sID);
    if (LeftCopy(mpFilePath, 4)=='Info') InfoLink();
    // Если это ссылка на torrserver, проверим, есть ли ещё хеш этого торрента
    if (HmsRegExMatch("--torrserver=([^\\s]+)", mpPodcastParameters, sServ) && HmsRegExMatch("/torrent/view/(\\w+)/", mpFilePath, sHash) && FileExists(mpComment)) {
      if (HmsRegExMatch2('^(.*?):(\\d+)', sServ, sServ, sVal)) nPort = StrToInt(sVal);
      sData = HmsSendRequest(sServ, '/torrent/cache', 'POST', 'application/json', '', '{"Hash":"'+sHash+'"}', nPort);
      if (!HmsRegExMatch('"Hash":"\\w+"', sData, '')) AddTorrentFile2TorrServer(mpComment);
    } 
    
    if (MediaResourceLink=='') MediaResourceLink = mpFilePath;
    if (Pos('.m3u8', MediaResourceLink)>0) MediaResourceLink = ' '+Trim(MediaResourceLink);
  }
}