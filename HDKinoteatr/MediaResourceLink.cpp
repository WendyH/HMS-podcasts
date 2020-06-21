// 2020.06.21
////////////////////////  Получение ссылки на поток ///////////////////////////
#define mpiJsonInfo 40032
#define mpiKPID     40033
#define DEBUG 0

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
int       gnTotalItems  = 0; 
TDateTime gStart        = Now;
string    gsAPIUrl      = "http://api.lostcut.net/hdkinoteatr/";
int       gnDefaultTime = 6000;
bool      gbQualityLog  = false;

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

///////////////////////////////////////////////////////////////////////////////
// Установка переменной Podcast: поиск родительской папки, содержащий скрипт
THmsScriptMediaItem GetRoot() {
  Podcast = PodcastItem;
  while ((Trim(Podcast[550])=='') && (Podcast.ItemParent!=nil)) Podcast=Podcast.ItemParent;
  return Podcast;
}

///////////////////////////////////////////////////////////////////////////////
// Раскодирование ссылки с сайта bazon.info
string BazonDecode(string data, string path) {
  if (ProgramVersion >= "3.0") {
    string s = 'decodeBase64=function(f){var g={},b=65,d=0,a,c=0,h,e="",k=String.fromCharCode,l=f.length;for(a="";91>b;)a+=k(b++);a+=a.toLowerCase()+"0123456789+/";for(b=0;64>b;b++)g[a.charAt(b)]=b;for(a=0;a<l;a++)for(b=g[f.charAt(a)],d=(d<<6)+b,c+=6;8<=c;)((h=d>>>(c-=8)&255)||a<l-2)&&(e+=k(h));return e};';
    data = jsEval(s+";(decodeBase64('"+data+"'))");
  } else {
    data = HmsBase64Decode(data);
  }
  string c = Copy(data, 1, 4);
  string d = Copy(data, 5, Length(data)-4);
  Variant e[256];
  int g, f = 0; string h = '';
  for (int k = 0; k < 256; k++) e[k] = k;
  for (k=0; k < 256; k++) {
    f = (f + e[k] + Ord(c[(k % Length(c))+1])) % 256;
    g = e[k];
    e[k] = e[f];
    e[f] = g;
  }
  k = 0; f = 0;
  for (int l = 0; l < Length(d); l++) {
    k = (k + 1   ) % 256;
    f = (f + e[k]) % 256;
    g = e[k];
    e[k] = e[f];
    e[f] = g;
    h += Chr(Ord(d[l+1]) ^ e[(e[k] + e[f]) % 256]);
  }
  return HmsUtf8Decode(ReplaceStr(HmsBase64Decode(h), "@", path));
}

///////////////////////////////////////////////////////////////////////////////
// Получение ссылки на Youtube
bool GetLink_Youtube33(string sLink) {
  string sData, sVideoID='', sAudio='', sSubtitlesLanguage='ru',
  sSubtitlesUrl, sFile, sVal, sMsg, sConfig, sHeaders, hlsUrl, subsUrl, jsUrl, 
  streamMap, algorithm, sType, itag, sig, alg, s, sp; TStringList QLIST;
  int i, n, w, num, height, priority, minPriority = 90, selHeight, maxHeight = 1080, audioQual;
  TJsonObject JSON, PLAYER_RESPONSE, VIDEO; TJsonArray FORMATS, SUBS; TRegExpr RegEx;
  
  sHeaders = 'Referer: '+sLink+'\r\n'+
             'User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.131 Safari/537.36\r\n'+
             'Origin: https://www.youtube.com\r\n';
  
  if (HmsRegExMatch('--maxheight=(\\d+)', mpPodcastParameters, sVal)) maxHeight = StrToInt(sVal);
  HmsRegExMatch('--sublanguage=(\\w{2})', mpPodcastParameters, sSubtitlesLanguage);
  bool bSubtitles = Pos('--subtitles' , mpPodcastParameters)>0;  
  bool bAdaptive  = Pos('--adaptive'  , mpPodcastParameters)>0;  
  bool bQualLog   = Pos('--qualitylog', mpPodcastParameters)>0;
  
  if (!HmsRegExMatch('[\\?&]v=([^&]+)'       , sLink, sVideoID))
    if (!HmsRegExMatch('youtu.be/([^&]+)'      , sLink, sVideoID))
    HmsRegExMatch('/(?:embed|v)/([^\\?]+)', sLink, sVideoID);
  
  if (sVideoID=='') return;
  
  sLink = 'https://www.youtube.com/watch?v='+sVideoID+'&hl=ru';
  sData = HmsDownloadURL(sLink, sHeaders, true);
  sData = HmsRemoveLineBreaks(sData);
  
  if (!HmsRegExMatch('player.config\\s*=\\s*({.*?});', sData, sConfig, 1, PCRE_SINGLELINE)) {
    HmsLogMessage(2 , mpTitle+': No player.config data in loaded page.'); 
    return; 
  }
  
  JSON = TJsonObject.Create();
  PLAYER_RESPONSE = TJsonObject.Create();
  QLIST = TStringList.Create();
  try {
    JSON.LoadFromString(sConfig);
    PLAYER_RESPONSE.LoadFromString(JSON.S['args\\player_response']);
    
    // Если есть субтитры и в дополнительных параметрах указано их показывать - загружаем 
    if (bSubtitles && PLAYER_RESPONSE.B['captions\\playerCaptionsTracklistRenderer\\captionTracks']) {
      string sTime1, sTime2, engSubs; float nStart, nDur;
      SUBS = PLAYER_RESPONSE.O['captions\\playerCaptionsTracklistRenderer\\captionTracks'].AsArray;
      for (i=0; i < SUBS.Length; i++) {
        if (SUBS[i].S['languageCode']==sSubtitlesLanguage) subsUrl = SUBS[i].S['baseUrl'];
        if (SUBS[i].S['languageCode']=='en'              ) engSubs = SUBS[i].S['baseUrl'];
      }
      if ((subsUrl=='') && (engSubs!='')) subsUrl = engSubs+'&tlang='+sSubtitlesLanguage;
      if (subsUrl!='') {
        sData = HmsDownloadURL(subsUrl+'&fmt=srv3', sHeaders, true);
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
        if (Trim(PodcastItem[mpiSubtitleLanguage])!='') bAdaptive = false;
        PodcastItem[mpiSubtitleLanguage] = sFile;
      }
    }
    
    hlsUrl = PLAYER_RESPONSE.S['streamingData\\hlsManifestUrl'];
    jsUrl  = JSON.S['assets\\js'];
    
    if (hlsUrl!='') {
      MediaResourceLink = ' '+hlsUrl;
      bAdaptive = false;
      sData = HmsDownloadUrl(hlsUrl, sHeaders, true);
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
          QLIST.Values[Format('%.4d', [height])] = sLink;
        } while (RegEx.SearchAgain());
      } finally { RegEx.Free(); }
      
    } else if (PLAYER_RESPONSE.B['streamingData\\formats']) {
      FORMATS = PLAYER_RESPONSE.O['streamingData\\formats'].AsArray;
      if (FORMATS[0].B['cipher'])
        algorithm = HmsDownloadURL('https://hms.lostcut.net/youtube/getalgo.php?jsurl='+HmsHttpEncode(jsUrl));
      for(i=0; i<FORMATS.Length; i++) {
        VIDEO = FORMATS[i];
        if (VIDEO.B['cipher']) {
          sLink = HmsHttpDecode(ExtractParam(VIDEO.S['cipher'], 'url', '', '&'));
          sig   = HmsHttpDecode(ExtractParam(VIDEO.S['cipher'], 's'  , '', '&'));
          sp    = HmsHttpDecode(ExtractParam(VIDEO.S['cipher'], 'sp' , '', '&'));
          if (sig!='') {
            for (w=1; w<=WordCount(algorithm, ' '); w++) {
              alg = ExtractWord(w, algorithm, ' ');
              if (Length(alg)<1) continue;
              if (Length(alg)>1) TryStrToInt(Copy(alg, 2, 4), num);
              if (alg[1]=='r') {s=''; for(n=Length(sig); n>0; n--) s+=sig[n]; sig = s;   } // Reverse
              if (alg[1]=='s') {sig = Copy(sig, num+1, Length(sig));                     } // Clone
              if (alg[1]=='w') {n = (num-Trunc(num/Length(sig)))+1; Swap(sig[1], sig[n]);} // Swap
            }
          }
          sLink += '&'+sp+'='+sig;
        } else {
          sLink = VIDEO.S['url'];
        }
        height = VIDEO.I['height'];
        if (height>0) QLIST.Values[Format('%.4d', [height])] = sLink;
        if (mpPodcastMediaFormats!='') {
          priority = HmsMediaFormatPriority(height, mpPodcastMediaFormats);
          if ((priority>=0) || (priority<minPriority)) {
            MediaResourceLink = sLink; minPriority = priority; selHeight = height;
          }
        } else if ((height>selHeight) && (height<= maxHeight)) {
          MediaResourceLink = sLink; selHeight = height;
          
        } else if ((height>=selHeight) && (height<= maxHeight) && (VIDEO.I['itag'] in ([18,22,37,38,82,83,84,85]))) {
          // Если высота такая же, но формат MP4 - то выбираем именно его (делаем приоритет MP4)
          MediaResourceLink = sLink; selHeight = height;
        }
      }
    } 
    if (bAdaptive || (MediaResourceLink=='') && PLAYER_RESPONSE.B['streamingData\\adaptiveFormats']) {
      FORMATS = PLAYER_RESPONSE.O['streamingData\\adaptiveFormats'].AsArray;
      if (FORMATS[0].B['cipher'] && (algorithm==''))
        algorithm = HmsDownloadURL('https://hms.lostcut.net/youtube/getalgo.php?jsurl='+HmsHttpEncode(jsUrl));
      for(i=0; i<FORMATS.Length; i++) {
        VIDEO = FORMATS[i];
        if (VIDEO.B['cipher']) {
          sLink = HmsHttpDecode(ExtractParam(VIDEO.S['cipher'], 'url', '', '&'));
          sig   = HmsHttpDecode(ExtractParam(VIDEO.S['cipher'], 's'  , '', '&'));
          sp    = HmsHttpDecode(ExtractParam(VIDEO.S['cipher'], 'sp' , '', '&'));
          if (sig!='') {
            for (w=1; w<=WordCount(algorithm, ' '); w++) {
              alg = ExtractWord(w, algorithm, ' ');
              if (Length(alg)<1) continue;
              if (Length(alg)>1) TryStrToInt(Copy(alg, 2, 4), num);
              if (alg[1]=='r') {s=''; for(n=Length(sig); n>0; n--) s+=sig[n]; sig = s;   } // Reverse
              if (alg[1]=='s') {sig = Copy(sig, num+1, Length(sig));                     } // Clone
              if (alg[1]=='w') {n = (num-Trunc(num/Length(sig)))+1; Swap(sig[1], sig[n]);} // Swap
            }
          }
          sLink += '&'+sp+'='+sig;
        } else {
          sLink = VIDEO.S['url'];
        }
        if (VIDEO.B['audioSampleRate'] && (audioQual < VIDEO.I['bitrate'])) {
          sAudio = sLink; audioQual = VIDEO.I['bitrate']; continue; 
        }
        height = VIDEO.I['height'];
        if (height>0) QLIST.Values[Format('%.4d', [height])] = sLink;
        if (mpPodcastMediaFormats!='') {
          priority = HmsMediaFormatPriority(height, mpPodcastMediaFormats);
          if ((priority>=0) || (priority<minPriority)) {
            MediaResourceLink = sLink; minPriority = priority; selHeight = height;
          }
        } else if ((height>selHeight) && (height<= maxHeight)) {
          MediaResourceLink = sLink; selHeight = height;
        }
      }
      sVal = ""; if (Trim(mpTimeStart)!="") sVal = " -ss "+mpTimeStart;
      if (bAdaptive && (sAudio!='')) {
        if (Pos('--downloadaudio', mpPodcastParameters)>0) {
          sFile = HmsTempDirectory+'\\'+PodcastItem.ItemID+'.webm';
          if (!FileExists(sFile))
            HmsDownloadURLToFile(sAudio, sFile, sHeaders, true);
        } else sFile = sAudio;
        MediaResourceLink = '-hide_banner -reconnect 1 -reconnect_at_eof 1 -reconnect_streamed 1 -reconnect_delay_max 2 -fflags +genpts -i "'+Trim(MediaResourceLink)+'"'+sVal+' -i "'+Trim(sFile)+'"';
      }
    }
    // Если еще не установлена реальная длительность видео - устанавливаем
    if ((Trim(mpTimeLength)=='') || (RightCopy(mpTimeLength, 6)=='00.000') && (hlsUrl=='')) {
      PodcastItem[mpiTimeLength] = HmsTimeFormat(PLAYER_RESPONSE.I['videoDetails\\lengthSeconds']);
    }
    if (bQualLog) {
      QLIST.Sort();
      sMsg = 'Доступное качество: ';
      for (i = 0; i < QLIST.Count; i++) {
        if (i>0) sMsg += ', ';
        sMsg += IntToStr(StrToInt(QLIST.Names[i])); // Обрезаем лидирующие нули
      }
      sMsg += '. Выбрано: ' + Str(selHeight);
      HmsLogMessage(1, mpTitle+'. '+sMsg);
    }
  } finally { JSON.Free; PLAYER_RESPONSE.Free; QLIST.Free; }
}

///////////////////////////////////////////////////////////////////////////////
// Показ видео сообщения
bool VideoMessage(string sMsg) {
  string sFileMP3 = HmsTempDirectory+'\\sa.mp3';
  string sFileImg = HmsTempDirectory+'\\youtubemsg_';
  sMsg = HmsHtmlToText(HmsJsonDecode(sMsg));
  int nH = cfgTranscodingScreenHeight;
  int nW = cfgTranscodingScreenWidth;
  HmsLogMessage(2, mpTitle+': '+sMsg);
  string sLink = Format('http://wonky.lostcut.net/videomessage.php?h=%d&w=%d&captsize=%d&fontsize=%d&caption=%s&msg=%s', [nH, nW, Round(nH/8), Round(nH/17), Podcast[mpiTitle], HmsHttpEncode(sMsg)]);
  HmsDownloadURLToFile(sLink, sFileImg);
  for (int i=1; i<=7; i++) CopyFile(sFileImg, sFileImg+Format('%.3d.jpg', [i]), false);
  try {
    if (!FileExists(sFileMP3)) HmsDownloadURLToFile('http://wonky.lostcut.net/mp3/sa.mp3', sFileMP3);
    sFileMP3 = '-i "'+sFileMP3+'" ';
  } except { sFileMP3 = ''; }
  MediaResourceLink = Format('%s-f image2 -r 1 -i "%s" -c:v libx264 -r 30 -pix_fmt yuv420p ', [sFileMP3, sFileImg+'%03d.jpg']);
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки на видео
THmsScriptMediaItem CreateMediaItem(THmsScriptMediaItem Folder, string sTitle, string sLink, string sImg, int nTime, string sGrp='') {
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, Folder.ItemID, sGrp);
  Item[mpiTitle     ] = sTitle;
  Item[mpiThumbnail ] = sImg;
  Item[mpiCreateDate] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0));
  Item[mpiTimeLength] = HmsTimeFormat(nTime)+'.000';
  if (HmsRegExMatch('(?:/embed/|v=)([\\w-_]{11})\\b', sLink, sImg))
    if (sImg!='movie')
    Item[mpiThumbnail ] = 'http://img.youtube.com/vi/'+sImg+'/0.jpg';
  gnTotalItems++;
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки-ошибки
void CreateErrorItem(string sMsg) {
  THmsScriptMediaItem Item = HmsCreateMediaItem('Err', PodcastItem.ItemID);
  Item[mpiTitle     ] = sMsg;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/icons/symbol-error.png';
}

///////////////////////////////////////////////////////////////////////////////
// Создание папки или подкаста
THmsScriptMediaItem CreateFolder(THmsScriptMediaItem Parent, string sName, string sLink, string sImg='') {
  THmsScriptMediaItem Item = Parent.AddFolder(sLink); // Создаём папку с указанной ссылкой
  Item[mpiTitle      ] = sName; // Присваиваем наименование
  Item[mpiThumbnail  ] = sImg;
  Item[mpiCreateDate ] = DateTimeToStr(IncTime(gStart, 0, -gnTotalItems, 0, 0)); // Для обратной сортировки по дате создания
  Item[mpiSeriesTitle] = mpSeriesTitle;
  Item[mpiYear       ] = mpYear;
  Item[mpiFolderSortOrder] = "-mpCreateDate";
  gnTotalItems++;               // Увеличиваем счетчик созданных элементов
  return Item;                  // Возвращаем созданный объект
}

///////////////////////////////////////////////////////////////////////////////
// Создание информационной ссылки
void CreateInfoItem(string sKey, string sVal) {
  THmsScriptMediaItem Item; sVal = Trim(sVal);
  if (sVal=="") return;
  CreateMediaItem(PodcastItem, sKey+': '+sVal, 'Info'+sVal, 'http://wonky.lostcut.net/vids/info.jpg', 7);
}

///////////////////////////////////////////////////////////////////////////////
// Формирование видео с картинкой с информацией о фильме
bool VideoPreview() {
  string sVal, sFileImage, sPoster, sTitle, sDescr, sCateg, sInfo, sLink, sData;
  int xMargin=7, yMargin=10, nSeconds=10, n, i; string sID, sField, sName;
  string gsCacheDir      = IncludeTrailingBackslash(HmsTempDirectory)+'hdkinoteatr/';
  string gsPreviewPrefix = "HDKinoteatr";
  float nH=cfgTranscodingScreenHeight, nW=cfgTranscodingScreenWidth;
  // Проверяем и, если указаны в параметрах подкаста, выставляем значения смещения от краёв
  if (HmsRegExMatch('--xmargin=(\\d+)', mpPodcastParameters, sVal)) xMargin=StrToInt(sVal);
  if (HmsRegExMatch('--ymargin=(\\d+)', mpPodcastParameters, sVal)) yMargin=StrToInt(sVal);
  
  if (Trim(PodcastItem.ItemOrigin.ItemParent[mpiJsonInfo])=='') return; // Если нет инфы - выходим быстро!
    
  // ---- Обпработка информации и создание отображаемого текста ------------
  TJsonObject JINFO = TJsonObject.Create();
  try {
    JINFO.LoadFromString(PodcastItem.ItemOrigin.ItemParent[mpiJsonInfo]);
    sTitle = JINFO.S['name'] + ' <c:#BB5E00>|<s:'+IntToStr(Round(cfgTranscodingScreenHeight/20))+'>' + JINFO.S['name_eng'];
    sID = JINFO.S['kpid'];
    if ((sID!='') && (sID!='0'))
      sPoster = 'https://st.kp.yandex.net/images/film_iphone/iphone360_'+sID+'.jpg';
    else 
      sPoster = HmsExpandLink(JINFO.S['image'], "http://hdkinoteatr.com");
    sData = 'country|Страна;year|Год;director|Режиссер;producer|Продюссер;scenarist|Сценарист;composer|Композитор;premiere|Премьера (мир);premiere_rf|Премьера (РФ);budget|Бюджет;actors|В ролях';
    sInfo = '';
    for (i=1; i<=WordCount(sData, ';'); i++) {
      HmsRegExMatch2('^(.*?)\\|(.*)', ExtractWord(i, sData, ';'), sField, sName);
      sVal = Trim(JINFO.S[sField]); if ((sVal=='') || (sVal=='0')) continue;
      if (sInfo!='') if (sField=='year') sInfo += '  '; else sInfo += '\n';
      sInfo += '<c:#FFC3BD>'+sName+': </c> '+sVal;
    }
    sCateg = JINFO.S['genre'];
    sDescr = JINFO.S['descr'];
  } finally { JINFO.Free; }
  // -----------------------------------------------------------------------
  
  TStrings INFO = TStringList.Create();       // Создаём объект TStrings
  if (sTitle=='') sTitle = ' ';
  ForceDirectories(gsCacheDir);
  sFileImage = ExtractShortPathName(gsCacheDir)+'videopreview_'; // Файл-заготовка для сохранения картинки
  sDescr = Copy(sDescr, 1, 3000); // Если блок описания получился слишком большой - обрезаем
  
  INFO.Text = ""; // Очищаем объект TStrings для формирования параметров запроса
  INFO.Values['prfx' ] = gsPreviewPrefix;  // Префикс кеша сформированных картинок на сервере
  INFO.Values['title'] = sTitle;           // Блок - Название
  INFO.Values['info' ] = sInfo;            // Блок - Информация
  INFO.Values['categ'] = sCateg;           // Блок - Жанр/категории
  INFO.Values['descr'] = sDescr;           // Блок - Описание фильма
  INFO.Values['mlinfo'] = '20';            // Максимальное число срок блока Info
  INFO.Values['w' ] = IntToStr(Round(nW)); // Ширина кадра
  INFO.Values['h' ] = IntToStr(Round(nH)); // Высота кадра
  INFO.Values['xm'] = IntToStr(xMargin);   // Отступ от краёв слева/справа
  INFO.Values['ym'] = IntToStr(yMargin);   // Отступ от краёв сверху/снизу
  INFO.Values['bg'] = 'http://www.pageresource.com/wallpapers/wallpaper/noir-blue-dark_3512158.jpg'; // Катринка фона (кэшируется на сервере) 
  INFO.Values['fx'] = '3'; // Номер эффекта для фона: 0-нет, 1-Blur, 2-more Blur, 3-motion Blur, 4-radial Blur
  INFO.Values['fztitle'] = IntToStr(Round(nH/14)); // Размер шрифта блока названия (тут относительно высоты кадра)
  INFO.Values['fzinfo' ] = IntToStr(Round(nH/22)); // Размер шрифта блока информации
  INFO.Values['fzcateg'] = IntToStr(Round(nH/26)); // Размер шрифта блока жанра/категории
  INFO.Values['fzdescr'] = IntToStr(Round(nH/18)); // Размер шрифта блока описания
  // Если текста описания больше чем нужно - немного уменьшаем шрифт блока
  if (Length(sDescr)>890) INFO.Values['fzdescr'] = IntToStr(Round(nH/20));
  // Если есть постер, задаём его параметры отображения (где, каким размером)
  if (sPoster!='') {
    INFO.Values['wpic'  ] = IntToStr(Round(nW/4)); // Ширина постера (1/4 ширины кадра)
    INFO.Values['xpic'  ] = '10';    // x-координата постера
    INFO.Values['ypic'  ] = '10';    // y-координата постера
    if (mpFilePath=='InfoUpdate') {
      INFO.Values['wpic'  ] = IntToStr(Round(nW/6)); // Ширина постера (1/6 ширины кадра)
      INFO.Values['xpic'  ] = IntToStr(Round(nW/2 - nW/12)); // центрируем
    }
    INFO.Values['urlpic'] = sPoster; // Адрес изображения постера
  }
  sData = '';  // Из установленных параметров формируем строку POST запроса
  for (n=0; n<INFO.Count; n++) sData += '&'+Trim(INFO.Names[n])+'='+HmsHttpEncode(INFO.Values[INFO.Names[n]]);
  INFO.Free(); // Освобождаем объект из памяти, теперь он нам не нужен
  // Делаем POST запрос не сервер формирования картинки с информацией
  sLink = HmsSendRequestEx('wonky.lostcut.net', '/videopreview.php?p='+gsPreviewPrefix, 'POST', 'application/x-www-form-urlencoded', '', sData, 80, 0, '', true);
  // В ответе должна быть ссылка на сформированную картинку
  if (LeftCopy(sLink, 4)!='http') {HmsLogMessage(2, 'Ошибка получения файла информации videopreview.'); return;}
  // Сохраняем сформированную картинку с информацией в файл на диске
  HmsDownloadURLToFile(sLink, sFileImage);
  // Копируем и нумеруем файл картики столько раз, сколько секунд мы будем её показывать
  for (n=1; n<=nSeconds; n++) CopyFile(sFileImage, sFileImage+Format('%.3d.jpg', [n]), false);
  // Для некоторых телевизоров (Samsung) видео без звука отказывается проигрывать, поэтому скачиваем звук тишины
  char sFileMP3 = ExtractShortPathName(HmsTempDirectory)+'\\silent.mp3';
  try {
    if (!FileExists(sFileMP3)) HmsDownloadURLToFile('http://wonky.lostcut.net/mp3/silent.mp3', sFileMP3);
    sFileMP3 = '-i "'+sFileMP3+'"';
  } except { sFileMP3=''; }
  // Формируем из файлов пронумерованных картинок и звукового команду для формирования видео
  MediaResourceLink = Format('%s -f image2 -r 1 -i "%s" -c:v libx264 -pix_fmt yuv420p ', [sFileMP3, sFileImage+'%03d.jpg']);
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылок на трейлеры по ID кинопоиска
void CreateTrailersLinks(string sLink) {
  string sHtml, sName, sTitle, sVal, sImg; TRegExpr RE1, RE2; THmsScriptMediaItem Item;
  
  sHtml = HmsDownloadURL(sLink, 'Referer: '+sLink, true);
  RE1 = TRegExpr.Create('<!-- ролик -->(.*?)<!-- /ролик -->', PCRE_SINGLELINE);
  RE2 = TRegExpr.Create('<a[^>]+href="/getlink.php[^>]+link=([^>]+\\.(mp4|mov|flv)).*?</a>', PCRE_SINGLELINE);
  try {
    if (RE1.Search(sHtml)) do {
      if (!HmsRegExMatch('(<a[^>]+href="/film/\\d+/video/\\d+/".*?</a>)', RE1.Match, sTitle)) continue;
      sTitle = HmsHtmlToText(sTitle);
      if (HmsRegExMatch('["\']previewFile["\']:\\s*?["\'](.*?)["\']', RE1.Match, sVal)) {
        sImg = 'http://kp.cdn.yandex.net/'+sVal;
      }
      if (RE2.Search(RE1.Match)) do {
        sLink = RE2.Match(1);
        sName = sTitle+' '+HmsHtmlToText(RE2.Match(0));
        Item  = PodcastItem.FindItemByProperty(mpiTitle, sName);
        if ((Item != null) && (Item != nil))
          Item[mpiFilePath] = sLink;
        else 
          CreateMediaItem(PodcastItem, sName, sLink, sImg, 230);
      } while (RE2.SearchAgain());
    } while (RE1.SearchAgain());
  } finally {  RE1.Free; RE2.Free; }
  if (gnTotalItems < 1) CreateErrorItem("На Кинопоиск.ру пока нет трейлеров этого фильма");
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
THmsScriptMediaItem GetItemWithInfo() {
  THmsScriptMediaItem Item = PodcastItem;
  while ((Trim(Item[mpiJsonInfo])=="") && (Item.ItemParent!=nil)) Item=Item.ItemParent;
  if (Trim(Item[mpiJsonInfo])=="") return nil;
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Пометка элемента как "Просмотренный"
void ItemMarkViewed(THmsScriptMediaItem Item) {
  string s1, s2;
  if (Pos('[П]', Item[mpiTitle])>0) return;
  if (HmsRegExMatch2('^(\\d+)(.*)', Item[mpiTitle], s1, s2))
    Item[mpiTitle] = s1 + ' [П] '+Trim(s2);
  else
    Item[mpiTitle] = '[П] '+Trim(Item[mpiTitle]);
}

///////////////////////////////////////////////////////////////////////////////
void FillVideoInfo(THmsScriptMediaItem Item) {
  string sData='', sSeason, sEpisode, sName, s1, s2;
  THmsScriptMediaItem Folder = GetItemWithInfo();
  
  if (Folder!=nil) sData = Folder[mpiJsonInfo];
  TJsonObject VIDEO = TJsonObject.Create();
  try {
    VIDEO.LoadFromString(sData);
    Item[mpiYear    ] = VIDEO.I['year'     ];
    Item[mpiGenre   ] = VIDEO.S['genre'    ];
    Item[mpiDirector] = VIDEO.S['director' ];
    Item[mpiComposer] = VIDEO.S['composer' ];
    Item[mpiActor   ] = VIDEO.S['actors'   ];
    Item[mpiAuthor  ] = VIDEO.S['scenarist'];
    Item[mpiComment ] = VIDEO.S['translation'];
    
    if (VIDEO.I['time']!=0) 
      Item[mpiTimeLength] = HmsTimeFormat(VIDEO.I['time'])+'.000';
    
    if (HmsRegExMatch2('season=(\\d+).*?episode=(\\d+)', Item[mpiFilePath], sSeason, sEpisode)) {
      Item[mpiSeriesSeasonNo ] = StrToInt(sSeason );
      Item[mpiSeriesEpisodeNo] = StrToInt(sEpisode);
      Item[mpiSeriesEpisodeTitle] = Item[mpiTitle];
      if (Trim(VIDEO.S['name_eng'])!="")
        Item[mpiSeriesTitle] = VIDEO.S['name_eng'];
      else
        Item[mpiSeriesTitle] = VIDEO.S['name'];
      
      if ((Folder!=nil) && (Pos('--markonplay', mpPodcastParameters)>0)) {
        if (Pos('s'+sSeason+'e'+sEpisode+';', Folder[mpiComment])>0)
          ItemMarkViewed(Item);
      }
      
    }
    
  } finally { VIDEO.Free; }
}

///////////////////////////////////////////////////////////////////////////////
// Нормализация имени серии
string NormName(string sName) {
  string sNum, sOther;
  sName = ReplaceStr(ReplaceStr(HmsHtmlToText(sName), '\r', ''), '\n', ' ');
  if (HmsRegExMatch2('^(\\d+)\\s*(.*)', sName, sNum, sOther)) {
    sName = Format('%.2d %s', [StrToInt(sNum), sOther]);
  }
  return sName;
}

///////////////////////////////////////////////////////////////////////////////
// Параметры:
// 1) Folder - Папка, где будут создаватся влоденные папки и элементы
// 2) sLink  - ссылка на файл плейлиста или сами данные JSON плейлиста
// 3) sName  - Имя подпапки, которую необходимо создать. Необязательный параметр.
// ---- Создание серий из плейлиста -------------------------------------------
void CreateSeriesFromPlaylist(THmsScriptMediaItem Folder, string sLink, string sName='') {
  string sData, s1, s2, s3, sUrl, sQual; int i; TJsonObject JSON, PLITEM; TJsonArray PLAYLIST; // Объявляем переменные
  
  // Если передано имя плейлиста, то создаём папку, в которой будем создавать элементы
  if (Trim(sName)!='') Folder = Folder.AddFolder(sName);
  
  // Если в переменной sLink сожержится знак '{', то там не ссылка, а сами данные Json
  if (Pos('{', sLink)>0) {
    sData = sLink;
  } else {
    sData = HmsDownloadURL(sLink, "Referer: "+mpFilePath, true);  // Загружаем плейлист
  }
  
  JSON  = TJsonObject.Create();                 // Создаём объект для работы с Json
  try {
    JSON.LoadFromString(sData);                 // Загружаем json данные в объект
    PLAYLIST = JSON.A['folder'];                // Пытаемся получить array с именем 'folder'
    if (PLAYLIST==nil) PLAYLIST = JSON.AsArray; // Если массив 'folder' получить не получилось, то представляем все наши данные как массив
    if (PLAYLIST!=nil) {                        // Если получили массив, то запускаем обход всех элементов в цикле
      for (i=0; i<PLAYLIST.Length; i++) {
        PLITEM = PLAYLIST[i];                   // Получаем текущий элемент массива
        sName = PLITEM.S['title'];              // Название - значение поля comment
        sLink = PLITEM.S['file' ];              // Получаем значение ссылки на файл
        
        // Форматируем числовое представление серий в названии
        // Если в названии есть число, то будет в s1 - то, что стояло перед ним, s2 - само число, s3 - то, что было после числа
        if (HmsRegExMatch3('^(.*?)(\\d+)(.*)$', sName, s1, s2, s3))
          sName = Format('%s %.2d %s', [s1, StrToInt(s2), s3]); // Форматируем имя - делаем число двухцифровое (01, 02...)
        
        // Проверяем, если это вложенный плейлист - запускаем создание элементов из этого плейлиста рекурсивно
        if (PLITEM.B['folder'])
          CreateSeriesFromPlaylist(Folder, PLITEM.S['folder'], sName);
        else {
          for (int n=1; n <= WordCount(sLink, ','); n++) {
            sUrl = ExtractWord(n, sLink, ','); sQual = "";
            HmsRegExMatch2('\\[(\\d+\\w+)\\](.*)', sUrl, sQual, sUrl);
            THmsScriptMediaItem Item = CreateMediaItem(Folder, sName, sUrl, mpThumbnail, gnDefaultTime, sQual); // Иначе просто создаём ссылки на видео
            Item[mpiComment] = mpFilePath; // Для bazon, запоминаем для Referer
          }
        }
      }
    } // end if (PLAYLIST!=nil)
    
  } finally { JSON.Free; }                      // Какие бы ошибки не случились, освобождаем объект из памяти
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылок на фильм или сериал
void CreateLinks() {
  string sHtml, sData, sName, sLink, sVal, sTrans, sTransID, sQual, sAdd, sSeasonName, sGrp, sAPI, sImg, sHeaders, sServ, js, path, hash, id;
  THmsScriptMediaItem Item; int i, n, nTime; TJsonObject VIDEO, PLAYLIST, VLINKS, PL, SEASON, TRANSLATION; TJsonArray SEASONS, EPISODES;
  VIDEO    = TJsonObject.Create(); TRegExpr RE;
  PLAYLIST = TJsonObject.Create();
  VLINKS   = TJsonObject.Create();
  PL       = TJsonObject.Create();
  if ((Trim(PodcastItem[mpiJsonInfo])=="") && HmsRegExMatch('.*/(\\d+)-', mpFilePath, id)) {
    PodcastItem[mpiJsonInfo] = HmsUtf8Decode(HmsDownloadURL(gsAPIUrl+"videos?id="+sVal));
    HmsRegExMatch("^\\[(.*)\\]$", PodcastItem[mpiJsonInfo], PodcastItem[mpiJsonInfo]);
  }
  try {
    VIDEO.LoadFromString(PodcastItem[mpiJsonInfo]);
    if ((mpThumbnail=="") && (VIDEO.I['kpid']!=0)) mpThumbnail = "https://st.kp.yandex.net/images/film_iphone/iphone360_"+VIDEO.S['kpid']+".jpg";
    
    gStart = ConvertToDate(VIDEO.S['date']); // Устанавливаем начальную точку для даты ссылок
    gnDefaultTime = VIDEO.I['time'];
    sLink = VIDEO.S['link'];
    sName = VIDEO.S['name'];
    
    try {
      VLINKS.LoadFromString(sLink);
      // --------------------------------------------------------------------
      try {
        if (VLINKS.B['bazon'] && (Pos('--bazon', mpPodcastParameters)>0)) {
          sLink = VLINKS.S['bazon\\iframe'];
          HmsRegExMatch("^(http.*?//[^/]+)", sLink, sServ);
          if (VIDEO.B['isserial']) {
            sHtml = HmsDownloadURL(sLink, 'Referer: '+sLink+'\r\nOrigin: '+sServ);
            if (HmsRegExMatch('<script>eval(\\(.*?\\))</script>', sHtml, js, 1, PCRE_SINGLELINE)) sData = jsEval(js);
            HmsRegExMatch('path:"(.*?)"', sData, path);
            if (HmsRegExMatch("eval(\\(.*?split\\('\\|'\\),0,{}\\)\\))", sData, js, 1, PCRE_SINGLELINE)) sData = jsEval(js);
            HmsRegExMatch('file="(.*?)";', sData, sVal);
            sVal = BazonDecode(ReplaceStr(sVal, '"+"', ""), path);
            mpFilePath = sLink;
            CreateSeriesFromPlaylist(PodcastItem, sVal, 'bazon');
          } else {
            sName = Trim('[bazon] '+VLINKS.S['bazon\\translate']+' '+VLINKS.S['bazon\\quality']);
            Item = CreateMediaItem(PodcastItem, sName, sLink, mpThumbnail, gnDefaultTime);
            FillVideoInfo(Item);
          }
        } // if (VLINKS.B['bazon'])
      } except { }
      // --------------------------------------------------------------------
      try {
        if (VLINKS.B['ustore'] && (Pos('--ustore', mpPodcastParameters)>0)) {
          sLink = VLINKS.S['ustore\\iframe'];
          HmsRegExMatch("^(http.*?//[^/]+)", sLink, sServ);
          if (VIDEO.B['isserial']) {
            sHtml = HmsUtf8Decode(HmsDownloadURL(sLink, 'Referer: '+mpFilePath+'\r\nOrigin: '+sServ));
            HmsRegExMatch('"hash"\\s*:\\s*"(.*?)"', sHtml, hash);
            HmsRegExMatch('"playlist":(.*?\\])\\s*\\}', sHtml, sData, 1, PCRE_SINGLELINE);
            HmsRegExMatch('(^.*/)', sLink, sLink);
            PLAYLIST.LoadFromString(sData);
            for (i=0; i < PLAYLIST.AsArray.Length; i++) {
              PL = PLAYLIST.AsArray[i];
              sTrans = PL.S['translate']; sTrans = Trim(ReplaceStr(sTrans, 'Не требуется', ''));
              PL = PL.O['data'];
              for (n=0; n < PL.Count; n++) {
                mpSeriesSeasonNo = PL.Names[n];
                sGrp = 'ustore '+Trim('Сезон '+mpSeriesSeasonNo+' '+sTrans);
                SEASON = PL.O[PL.Names[n]];
                for (int q=0; q < SEASON.Count ; q++) {
                  mpSeriesEpisodeNo = SEASON.Names[q];
                  id = SEASON.S[mpSeriesEpisodeNo];
                  Item = CreateMediaItem(PodcastItem, Format('%.2d cерия', [StrToInt(mpSeriesEpisodeNo)]), sLink+id, mpThumbnail, gnDefaultTime, sGrp);
                  Item.ItemParent[mpiFolderSortOrder] = 'mpTitle';
                  Item.ItemParent.Sort('+mpTitle');
                }
              }
            }
          } else {
            sName = Trim('[ustore] '+VLINKS.S['ustore\\translate']+' '+VLINKS.S['ustore\\quality']);
            Item = CreateMediaItem(PodcastItem, sName, sLink, mpThumbnail, gnDefaultTime);
            FillVideoInfo(Item);
          }            
        } // if (VLINKS.B['ustore'])
      } except { }
      // --------------------------------------------------------------------
      try {
        if (VLINKS.B['alloha'] && (Pos('--alloha', mpPodcastParameters)>0)) {
          sLink = VLINKS.S['alloha\\iframe'];
          HmsRegExMatch("^(http.*?//[^/]+)", sLink, sServ);
          if (VIDEO.B['isserial']) {
            sHtml = HmsUtf8Decode(HmsDownloadURL(sLink, 'Referer: '+mpFilePath));
            HmsRegExMatch("serial\\s*=\\s*'(.*?)'", sHtml, sVal, 1, PCRE_SINGLELINE);
            PLAYLIST.LoadFromString(sVal);
            for (i=0; i < PLAYLIST.Count; i++) {
              SEASON = PLAYLIST.O[PLAYLIST.Names[i]];
              for (n=0; n < SEASON.Count; n++) {
                mpSeriesEpisodeNo = SEASON.Names[n];
                PL = SEASON.O[SEASON.Names[n]];
                for (int w=0; w < PL.Count; w++) {
                  TRANSLATION = PL.O[PL.Names[w]];
                  sVal   = TRANSLATION.S['player'];
                  sData  = AllohaDecode(sVal);
                  HmsRegExMatch('"file":"(.*?)"', sData, sVal);
                  sLink  = AllohaDecode(sVal);
                  sGrp = Trim('alloha Сезон '+PLAYLIST.Names[i]+' '+ReplaceStr(TRANSLATION.S['translation'], 'Не требуется', ''));
                  Item = CreateMediaItem(PodcastItem, Format('%.2d cерия', [StrToInt(mpSeriesEpisodeNo)]), sLink, mpThumbnail, gnDefaultTime, sGrp);
                  Item.ItemParent[mpiFolderSortOrder] = 'mpTitle';
                  Item.ItemParent.Sort('+mpTitle');
                }
              }
            }
          } else {
            sName = Trim('[alloha] '+VLINKS.S['alloha\\translate']+' '+VLINKS.S['alloha\\quality']);
            Item = CreateMediaItem(PodcastItem, sName, sLink, mpThumbnail, gnDefaultTime);
            FillVideoInfo(Item);
          }
        } // if (VLINKS.B['alloha'])
      } except { }
      // --------------------------------------------------------------------
      try {
        if (VLINKS.B['videocdn'] && (Pos('--videocdn', mpPodcastParameters)>0)) {
          sLink = VLINKS.S['videocdn\\iframe'];
          sName = Trim('[videocdn] '+VLINKS.S['videocdn\\translate']+' '+VLINKS.S['videocdn\\quality']);
          if (VIDEO.B['isserial']) {
            sHtml = HmsUtf8Decode(HmsDownloadURL("https://hms.lostcut.net/proxy.php?"+sLink, 'Referer: '+mpFilePath));
            // Собираем информацию о переводах
            TStringList TRANS = TStringList.Create();
            if (HmsRegExMatch('(<div[^>]+translations.*?</div>)', HmsRemoveLineBreaks(sHtml), sData)) {
              RE = TRegExpr.Create('<option[^>]+value="(.*?)".*?</option>', PCRE_SINGLELINE);
              if (RE.Search(sData)) do {
                TRANS.Values[RE.Match(1)] = HmsHtmlToText(RE.Match(0));
              } while (RE.SearchAgain());
            }
            HmsRegExMatch('id="files"\\s*value="(.*?)"', sHtml, sVal);
            PLAYLIST.LoadFromString(HmsHtmlDecode(sVal));
            for (int nTrans=0; nTrans<PLAYLIST.Count; nTrans++) {
              sTransID = PLAYLIST.Names[nTrans];
              sTrans   = TRANS.Values[sTransID]; // Название озвучки
              sVal =  Html5Decode(PLAYLIST.S[sTransID]);
              PL.LoadFromString(sVal);
              SEASONS = PL.AsArray();
              for (i=0; i<SEASONS.Length; i++) {
                EPISODES = SEASONS[i].A['folder'];
                if (EPISODES==nil) {
                  // Нет сезонов - есть только серии
                  EPISODES = SEASONS;
                  sSeasonName = '';
                } else {
                  sSeasonName = NormName(SEASONS[i].S['comment']);
                  EPISODES = SEASONS[i].A['folder'];
                }
                for (n=0; n<EPISODES.Length; n++) {
                  sData = EPISODES[n].S['file'];
                  sName = NormName(EPISODES[n].S['comment']);
                  RE = TRegExpr.Create('\\[(.*?)\\]([^,\\s"\']+)');
                  if (RE.Search(sData)) do {
                    sQual = RE.Match(1);
                    sLink = 'http:'+RE.Match(2);
                    sGrp  = Trim(Trim('Videocdn '+sTrans)+' '+sQual)+'\\'+sSeasonName;
                    Item = CreateMediaItem(PodcastItem, sName, sLink, mpThumbnail, gnDefaultTime, sGrp);
                  } while (RE.SearchAgain());
                } //for (n=0; n<EPISODES.Length; n++)
              } //for (i=0; i<SEASONS.Length; i++)
            } //for (int nTrans=0; nTrans<PLAYLIST.Count; nTrans++)
            RE.free; TRANS.Free;
          } else {
            Item = CreateMediaItem(PodcastItem, sName, sLink, mpThumbnail, gnDefaultTime);
            FillVideoInfo(Item);
          }
        } // if (VLINKS.B['videocdn'])
      } except { }
      // --------------------------------------------------------------------
      try {
        if (VLINKS.B['collaps'] && (Pos('--collaps', mpPodcastParameters)>0)) {
          if (VIDEO.B['isserial']) {
            sHtml = HmsUtf8Decode(HmsDownloadURL(VLINKS.S['collaps\\iframe'], 'Referer: '+mpFilePath));
            //HmsRegExMatch('MakePlayer\\(({.*?})\\);', sHtml, sData);
            HmsRegExMatch('playlist:\\s*({.*?}]}]\\s*}),', sHtml, sData);
            PLAYLIST.LoadFromString(sData);
            SEASONS = PLAYLIST.O['seasons'].AsArray();
            for (i=0; i<SEASONS.Length; i++) {
              sSeasonName = SEASONS[i].S['season']+' сезон';
              EPISODES = SEASONS[i].O['episodes'].AsArray();
              for (n=0; n<EPISODES.Length; n++) {
                sImg  = EPISODES[n].S['mini'];
                nTime = EPISODES[n].I['duration'];
                sVal  = EPISODES[n].S['episodeName']; if (sVal=='') sVal = 'серия';
                sName = Format('%.2d %s', [EPISODES[n].I['episode'], sVal]);
                PLAYLIST = EPISODES[n].O['hlsList'];
                for (q=0; q<PLAYLIST.Count; q++) {
                  sQual = PLAYLIST.Names[q];
                  sLink = PLAYLIST.S[sQual];
                  sGrp = 'Collaps '+sQual+'\\'+sSeasonName;
                  Item = CreateMediaItem(PodcastItem, sName, sLink, sImg, nTime, sGrp);
                }
              }
            }
          } else {
            Item = CreateMediaItem(PodcastItem, '[collaps] '+VIDEO.S['name'], VLINKS.S['collaps\\iframe'], mpThumbnail, gnDefaultTime);
            FillVideoInfo(Item);
          }
          
        } // if (VLINKS.B['collaps'])
      } except { }
      // --------------------------------------------------------------------
      try {
        if (VLINKS.B['iframe'] && (Pos('--iframe', mpPodcastParameters)>0)) {
          if (VIDEO.B['isserial']) {
            sHtml = HmsUtf8Decode(HmsDownloadURL(VLINKS.S['iframe\\iframe'], 'Referer: '+mpFilePath));
            //HmsRegExMatch('background:\\s*url\\((https.*?)\\)', sHtml, sImg);
            RE = TRegExpr.Create('<a[^>]+href=[\'"](.*?)[\'"][^>]*>Сезон\\s*\\d+</a>', PCRE_CASELESS);
            if (RE.Search(sHtml)) do {
              sSeasonName = NormName(RE.Match(0));
              if (Pos(RE.Match, VLINKS.S['iframe\\iframe'])>0) 
                sData = sHtml;
              else 
                sData = HmsUtf8Decode(HmsDownloadURL('https://videoframe.at'+RE.Match, 'Referer: '+mpFilePath));
              TRegExpr RESeries = TRegExpr.Create('<a[^>]+href=[\'"](.*?)[\'"][^>]*>\\d+\\s*серия</a>', PCRE_CASELESS);
              if (RESeries.Search(sData)) do {
                sLink = 'https://videoframe.at'+RESeries.Match(1);
                sName = NormName(RESeries.Match(0));
                sGrp  = 'iframe\\'+sSeasonName;
                Item = CreateMediaItem(PodcastItem, sName, sLink, mpThumbnail, gnDefaultTime, sGrp);
              } while (RESeries.SearchAgain());
              Item = PodcastItem.AddFolder(sGrp);
              if (Item!=nil) Item.Sort('+mpTitle');
            } while (RE.SearchAgain());
          } else {
            Item = CreateMediaItem(PodcastItem, '[iframe] '+VIDEO.S['name'], VLINKS.S['iframe\\iframe'], mpThumbnail, gnDefaultTime);
            FillVideoInfo(Item);
          }
          
        } // if (VLINKS.B['iframe'])
      } except { }
      // --------------------------------------------------------------------
      try {
        if (VLINKS.B['hdvb'] && (Pos('--hdvb', mpPodcastParameters)>0)) {
          if (VIDEO.B['isserial']) {
            string sSeriesData, sOriginalLink;
            sOriginalLink = VLINKS.S['hdvb\\iframe'];
            sHtml = HmsUtf8Decode(HmsDownloadURL(sOriginalLink, 'Referer: '+mpFilePath, true));
            HmsRegExMatch('name="translation"><option value="([^"]+)" selected', sHtml, sTrans);
            // Есть зесозы? Если нет - деалем так, чтобы нашёлся один, текущий
            if (!HmsRegExMatch('(<select[^>]+id="seasons".*?</select>)', sHtml, sData)) {
              sData = "<option value="" selected></option>"; 
            }
            RE = TRegExpr.Create('<option[^>]+value="(.*?)".*?</option>', PCRE_SINGLELINE);
            try {
              if (RE.Search(sData)) do {
                mpSeriesSeasonNo = RE.Match;
                if (Pos('selected', RE.Match(0))>0)
                  sSeriesData = sHtml;
                else
                  sSeriesData = HmsUtf8Decode(HmsDownloadURL(sOriginalLink+'?s='+mpSeriesSeasonNo+'&t='+sTrans, 'Referer: '+mpFilePath, true));
                HmsRegExMatch('(<select[^>]+id="episodes".*?</select>)', sSeriesData, sSeriesData);
                RESeries = TRegExpr.Create('<option[^>]+value="(\\d+)".*?</option>', PCRE_SINGLELINE);
                if (RESeries.Search(sSeriesData)) do {
                  sLink = VLINKS.S['hdvb\\iframe']+'?e='+RESeries.Match+'&s='+mpSeriesSeasonNo+'&t='+sTrans;
                  sName = NormName(RESeries.Match(0));
                  sGrp  = 'hdvb '+VLINKS.S['hdvb\\translate']+'\\Сезон '+mpSeriesSeasonNo;
                  Item = CreateMediaItem(PodcastItem, sName, sLink, mpThumbnail, gnDefaultTime, sGrp);
                } while (RESeries.SearchAgain());
              } while (RE.SearchAgain());
            } finally { RE.Free; RESeries.Free; }
          } else {
            sName = '[hdvb] '+VLINKS.S['hdvb\\translate']+' '+VLINKS.S['hdvb\\quality'];
            Item  = CreateMediaItem(PodcastItem, sName, VLINKS.S['hdvb\\iframe'], mpThumbnail, gnDefaultTime);
            FillVideoInfo(Item);
          }
          
        } // if (VLINKS.B['hdvb'])
      } except { }
      // --------------------------------------------------------------------
      try {
        if (VLINKS.B['kodik'] && (Pos('--kodik', mpPodcastParameters)>0)) {
          if (HmsRegExMatch2('kodik\\.\\w+/(\\w+)/(\\d+)/', VLINKS.S['kodik\\iframe'], sName, sVal)) {
            if (sName=='video') sName = 'movie';
            sData = HmsUtf8Decode(HmsDownloadURL('https://kodikapi.com/search?token=b7cc4293ed475c4ad1fd599d114f4435&id='+sName+'-'+sVal+'&with_material_data=true&with_seasons=true&with_episodes=true', 'Referer: '+mpFilePath));
            string sEpisode, sSName; TStringList SERIES;
            PLAYLIST  = TJsonObject.Create();
            SERIES = TStringList.Create();
            PLAYLIST.LoadFromString(sData);
            PLAYLIST = PLAYLIST.O['results\\0'];
            nTime = PLAYLIST.I['material_data\\duration'] * 60;
            if (PLAYLIST.B['seasons']) {
              // Это сериал - создаём сезоны и серии
              sTrans = 'kodik '+VLINKS.S['kodik\\translate']+' '+VLINKS.S['kodik\\quality'];
              for (i=0; i < PLAYLIST.O['seasons'].Count; i++) {
                sSName = PLAYLIST.O['seasons'].Names[i];
                SEASON = PLAYLIST.O['seasons\\'+sSName];
                if ((PLAYLIST.O['seasons'].Count>1) || (!PLAYLIST.B['seasons\\1\\episodes']) || (PLAYLIST.O['seasons\\1\\episodes'].Count>15)) {
                  sGrp = sTrans+'\\'+Format('%.2d сезон', [StrToInt(sSName)]);
                }
                for (n=0; n < SEASON.O['episodes'].Count; n++) {
                  sEpisode = SEASON.O['episodes'].Names[n];
                  if (SERIES.Count > 99) sName = Format('%.3d', [StrToInt(sEpisode)]);
                  else                   sName = Format('%.2d', [StrToInt(sEpisode)]);
                  SERIES.Values[sName] = SEASON.S['episodes\\'+sEpisode];
                }
                SERIES.Sort();
                for (n=0; n < SERIES.Count; n++) {
                  sName = SERIES.Names[n];
                  CreateMediaItem(PodcastItem, sName+' серия', "http:"+SERIES.Values[sName], mpThumbnail, nTime, sGrp);
                }
              }
              
            } else {
              // Это фильм - создаём одну ссылку на видео
              Item = CreateMediaItem(PodcastItem, '[kodik] '+PLAYLIST.S['title'], VLINKS.S['kodik\\iframe'], mpThumbnail, nTime);
              FillVideoInfo(Item);
            }
          }
        } // if (VLINKS.B['kodik'])
      } except { }
      // --------------------------------------------------------------------
      try {
        if (VLINKS.B['ONIK'] && (Pos('--onik', mpPodcastParameters)>0)) {
          sOriginalLink = VLINKS.S['ONIK\\iframe'];
          HmsRegExMatch('^(.*?//.*?)/', sOriginalLink, sServ);
          if (VIDEO.B['isserial']) {
            sHtml = HmsUtf8Decode(HmsDownloadURL(sOriginalLink, 'Referer: '+mpFilePath, true));
            RE = TRegExpr.Create('(<div[^>]+class="item-poster".*?</div>)', PCRE_SINGLELINE);
            if (RE.Search(sHtml)) do {
              sSName = ""; sImg = ""; sLink="";
              HmsRegExMatch('data-poster="(.*?)"', RE.Match, sImg  );
              HmsRegExMatch('data-season="(.*?)"', RE.Match, sSName);
              HmsRegExMatch('href="(.*?)"'       , RE.Match, sLink );
              HmsRegExMatch('title="(.*?)"'      , RE.Match, sName );
              if (sLink=="") continue;
              sName = NormName(sName);
              sGrp  = 'ONIK\\'+sSName;
              Item = CreateMediaItem(PodcastItem, sName, sServ+sLink, mpThumbnail, gnDefaultTime, sGrp);
            } while (RE.SearchAgain());
            RE.Free;
          } else {
            sName = Trim('[ONIK] '+VLINKS.S['ONIK\\translate']+' '+VLINKS.S['ONIK\\quality']);
            Item  = CreateMediaItem(PodcastItem, sName, sOriginalLink, mpThumbnail, gnDefaultTime);
            FillVideoInfo(Item);
          }
          
          
        } // if (VLINKS.B['ONIK'])
      } except { }
      // --------------------------------------------------------------------
      if (VLINKS.B['trailer']) {
        sLink = VLINKS.S['trailer\\iframe'];
        Item = CreateMediaItem(PodcastItem, 'Трейлер', sLink, mpThumbnail, gnDefaultTime);
        if (HmsRegExMatch('/embed/([^/]+)', sLink, sVal))
          Item[mpiThumbnail] = 'http://img.youtube.com/vi/'+sVal+'/0.jpg';
        Item[mpiTimeLength] = '00:03:30';
      }
    } except { HmsLogMessage(1, "В ссылках на фильм содержаться плохие данные. Кина не будет. "+sLink); }
    
    // Специальная папка добавления/удаления в избранное
    if (VIDEO.B['isserial'] && (Pos('--controlfavorites', mpPodcastParameters)>0)) {
      // Проверка, находится ли сериал в избранном?
      Item = HmsFindMediaFolder(Podcast.ItemID, 'favorites');
      if (Item!=nil) {
        bool bExist = HmsFindMediaFolder(Item.ItemID, mpFilePath) != nil;
        if (bExist) { sName = "Удалить из избранного"; sLink = "-FavDel="+PodcastItem.ItemParent.ItemID+";"+mpFilePath; }
        else        { sName = "Добавить в избранное" ; sLink = "-FavAdd="+PodcastItem.ItemParent.ItemID+";"+mpFilePath; }
        CreateMediaItem(PodcastItem, sName, sLink, 'http://wonky.lostcut.net/icons/ok.png', 1, sName);
      }
    } 
    
    // Создание информационных ссылок
    if (Pos('--infoitems', mpPodcastParameters) > 0) {
      CreateInfoItem('Страна'  , VIDEO.S["country" ]);
      CreateInfoItem('Жанр'    , VIDEO.S["genre"   ]);
      CreateInfoItem('Год'     , VIDEO.S["year"    ]);
      CreateInfoItem('Режиссёр', VIDEO.S["director"]);
      if (VIDEO.B["rating_kp"  ]) CreateInfoItem('Рейтинг КП'  , VIDEO.S["rating_kp"  ]+' ('+VIDEO.S["rating_kp_votes"  ]+')');
      if (VIDEO.B["rating_imdb"]) CreateInfoItem('Рейтинг IMDb', VIDEO.S["rating_imdb"]+' ('+VIDEO.S["rating_imdb_votes"]+')');
      CreateInfoItem('Перевод' , VIDEO.S["translation"]);
    }
    
  } finally { VIDEO.Free; PLAYLIST.Free; VLINKS.Free; PL.Free; }
}

///////////////////////////////////////////////////////////////////////////////
// Декодирование ссылок для HTML5 плеера
string hd0_decode(string data) {
  if ((data=="") || (Pos(".", data) > 0)) return data;
  if (ProgramVersion >= "3.0") {
    string s = 'decodeBase64=function(f){var g={},b=65,d=0,a,c=0,h,e="",k=String.fromCharCode,l=f.length;for(a="";91>b;)a+=k(b++);a+=a.toLowerCase()+"0123456789+/";for(b=0;64>b;b++)g[a.charAt(b)]=b;for(a=0;a<l;a++)for(b=g[f.charAt(a)],d=(d<<6)+b,c+=6;8<=c;)((h=d>>>(c-=8)&255)||a<l-2)&&(e+=k(h));return e};';
    data = jsEval(s+";(decodeBase64('"+data+"'))");
  } else {
    data = HmsBase64Decode(data);
  }
  string ch, sDecoded = "";
  for (int i=1; i <= Length(data); i++) {
    sDecoded += "\\u00" + HmsStringToHex(Copy(data, i, 1));
  }
  return HmsJsonDecode(sDecoded);
}

///////////////////////////////////////////////////////////////////////////////
// Декодирование ссылок для HTML5 плеера
string Html5Decode(string sEncoded) {
  if ((sEncoded=="") || (Pos(".", sEncoded) > 0)) return sEncoded;
  if (sEncoded[1]!="#") return sEncoded;
  sEncoded = Copy(sEncoded, 2, Length(sEncoded)-1);
  string sDecoded = "";
  for (int i=1; i <= Length(sEncoded); i+=3) {
    sDecoded += "\\u0" + Copy(sEncoded, i, 3);
  }
  return HmsJsonDecode(sDecoded);
}

///////////////////////////////////////////////////////////////////////////////
string CryptoJsAesDecrypt(string pass, string ct, string iv, string salt) {
  string concatedPassphrase = pass+HmsHexToString(salt);
  string s1   = HmsHexToString(HmsMD5SumOfString(concatedPassphrase));
  string s2   = HmsHexToString(HmsMD5SumOfString(s1+concatedPassphrase));
  string s3   = HmsHexToString(HmsMD5SumOfString(s2+concatedPassphrase));
  string key  = LeftCopy(s1+s2+s3, 32);
  string text = HmsCryptCipherDecode("Rijndael", ct, key, HmsHexToString(iv), cmCBCx, "MIME64");
  text = HmsJsonDecode(Trim(text));
  HmsRegExMatch('^"(.*?)"$', text, text, 1, PCRE_SINGLELINE);
  return text;
}

///////////////////////////////////////////////////////////////////////////////
// Расшифровка текста плеера playerjs-alloha-new с allohastream.com
string AllohaDecode(string sData) {
  string pre, salt, iv, ct; int i;
  Variant trash = ["##P3w7Xl58Kj4qPj8/Pl58Xjx8Pnw/ISrihJYofDshP17ihJY+", "##Pzs+KSEoKjt8fD58KjxefCp8XipgPj98KHwqPnx8fl1bfD58Kl4q", "##PGBeKmAqPnzihJYqKuKEll0/Wyo7fHw+fCrihJY7Xipg4oSWKj4=", "##fFs+KuKElj5eP1s7fHw+fCo8KirihJZdfHxePCoqfA==", "##OyE/XuKElj4qXipgfHxePCrihJZ8fF4qYF4qKnzihJYqfl1bfD58"];
  for (i=0; i < Length(trash) ; i++) sData = ReplaceStr(sData, trash[i], "");
  for (i=0; i < Length(trash) ; i++) sData = ReplaceStr(sData, trash[i], ""); // Иногда мусор встраивается в мусор, поэтому проходим два раза
  pre = LeftCopy(sData, 2);
  if (pre=="#9") {
    return hd0_decode(Copy(sData, 3, Length(sData)));
  }
  if (pre=="#6") {
    salt = Copy(sData, Length(sData)-15, 16);
    iv   = Copy(sData, Length(sData)-49, 32);
    ct   = Copy(sData, 3, Length(sData)-54);
    return CryptoJsAesDecrypt('vG~N:=!d~Nhkn=k^)}_>F*zvTD=~ffZ+3pE!WCY4>X!QJY4>X!QJsuvu1HFvP_rE^Ny', ct, iv, salt);
  }
  if (pre=="#0") {
      return HmsBase64Decode(Copy(sData, 3, Length(sData)));    
  }
  return sData;
}

///////////////////////////////////////////////////////////////////////////////
// Получение ссылки с ресурса allohastream.com
void GetLink_Allohastream(string sLink, string sHeaders="Referer: http://www.hdkinoteatr.com/") {
  string html, data, sHeight, sQual, sSelectedQual, sTransID, sTrans, sSeasonName; int i, n, iPriority, iMinPriority=99;
  html = HmsUtf8Decode(HmsDownloadURL(sLink, sHeaders));
  HmsRegExMatch('Playerjs\\("(.*?)"', html, data);
  data = AllohaDecode(data);
  HmsRegExMatch('"file":"(.*?)"', data, MediaResourceLink);
  MediaResourceLink = AllohaDecode(MediaResourceLink);
  return;
}

///////////////////////////////////////////////////////////////////////////////
// Получение ссылки с ресурса tvmovies
void GetLink_tvmovies(string sLink, string sHeaders="") {
  string html, data, sHeight, sQual, sSelectedQual, sTransID, sTrans, sSeasonName; int i, n, iPriority, iMinPriority=99;
  
  html = HmsUtf8Decode(HmsDownloadURL('https://hms.lostcut.net/proxy.php?'+sLink, sHeaders));
  if (HmsRegExMatch('Playerjs\\("(.*?)"', html, data)) {
    if (LeftCopy(data, 1)=='#') data = Html5Decode(data);
    MediaResourceLink = data;
    return;
  }
  // Собираем информацию о переводах
  TStringList TRANS = TStringList.Create();
  TStringList QLIST = TStringList.Create();
  if (HmsRegExMatch('(<div[^>]+translations.*?</div>)', HmsRemoveLineBreaks(html), data)) {
    TRegExpr RE = TRegExpr.Create('<option[^>]+value="(.*?)".*?</option>', PCRE_SINGLELINE);
    if (RE.Search(data)) do {
      TRANS.Values[RE.Match(1)] = HmsHtmlToText(RE.Match(0));
    } while (RE.SearchAgain());
  }
  HmsRegExMatch('id="files"\\s*value=[\'"](.*?)[\'"]', html, data);
  TJsonObject PLAYLIST = TJsonObject.Create();
  PLAYLIST.LoadFromString(HmsHtmlDecode(data));
  for (int nTrans=0; nTrans<PLAYLIST.Count; nTrans++) {
    sTransID = PLAYLIST.Names[nTrans];
    sTrans   = TRANS.Values[sTransID]; // Название озвучки
    data = Html5Decode(PLAYLIST.S[sTransID]);
    QLIST.Clear();
    RE = TRegExpr.Create('\\[(\\d+).*?\\]([^,\\s"\']+)');
    if (RE.Search(data)) do {
      sQual = RE.Match(1);
      sLink = 'http:'+RE.Match(2);
      sHeight = Format('%.5d', [StrToInt(sQual)]);
      QLIST.Values[sHeight] = sLink;
      MediaResourceLink = " "+sLink; // по-умолчанию
      // Если установлен ключ --quality или в настройках подкаста выставлен приоритет выбора качества
      // ------------------------------------------------------------------------
      iPriority = HmsMediaFormatPriority(StrToInt(sHeight), mpPodcastMediaFormats);
      if ((iPriority >= 0) && (iPriority < iMinPriority)) {
        iMinPriority  = iPriority;
        sSelectedQual = sHeight;
      }
    } while (RE.SearchAgain());
    if (Pos(sTrans, mpTitle)>0) break;
  } //for (int nTrans=0; nTrans<PLAYLIST.Count; nTrans++)
  RE.free; TRANS.Free;
  HmsRegExMatch('--quality=(\\w+)', mpPodcastParameters, sQual);
  QLIST.Sort();
  if (QLIST.Count > 0) {
    if      (sQual=='low'   ) sSelectedQual = QLIST.Names[0];
    else if (sQual=='medium') sSelectedQual = QLIST.Names[Round((QLIST.Count-1) / 2)];
    else if (sQual=='high'  ) sSelectedQual = QLIST.Names[QLIST.Count - 1];
    else if (HmsRegExMatch('(\\d+)', sQual, sQual)) {
      extended minDiff = 999999; // search nearest quality
      for (i=0; i < QLIST.Count; i++) {
        extended diff = StrToInt(QLIST.Names[i]) - StrToInt(sQual);
        if (Abs(diff) < minDiff) {
          minDiff = Abs(diff);
          sSelectedQual = QLIST.Names[i];
        }
      }
    }
  }
  if (sSelectedQual!='') MediaResourceLink = ' '+QLIST.Values[sSelectedQual];
  
  bool bQualLog = Pos('--qualitylog', mpPodcastParameters) > 0;
  if (bQualLog) {
    string sMsg = 'Доступное качество: ';
    for (i = 0; i < QLIST.Count; i++) {
      if (i>0) sMsg += ', ';
      sMsg += IntToStr(StrToInt(QLIST.Names[i])); // Обрезаем лидирующие нули
    }
    if (sSelectedQual != '') sSelectedQual = IntToStr(StrToInt(sSelectedQual));
    else sSelectedQual = 'Auto';
    sMsg += '. Выбрано: ' + sSelectedQual;
    HmsLogMessage(1, mpTitle+'. '+sMsg);
  }
  
  QLIST.Free;
}

///////////////////////////////////////////////////////////////////////////////
// Декодирование ссылок с ресурса ustore.bz (u-cdn.top)
string UstoreDecode(string data) {
  if (data=="") return "";
  if (data[1]=="=") {
    data = Copy(data, 2, Length(data)-1);
    string s1 = "qsefthzxQSEFTHZX01234";
    string s2 = "plijymknPLIJYMKN98765";
    for (int i=1; i <= Length(s1); i++) {
      data = ReplaceStr(data, s1[i],  "__");
      data = ReplaceStr(data, s2[i], s1[i]);
      data = ReplaceStr(data,  "__", s2[i]);
    }
    data = HmsHttpDecode(HmsBase64Decode(data));
    s1 = "WO87FXYEZP4abQ2cdR0efS9ghTHijUK";
    s2 = "k6lBmCnJoMpGq3rAsLt1uNv5wDxIyVz";
    for (i=1; i <= Length(s1); i++) {
      data = ReplaceStr(data, s1[i],  "__");
      data = ReplaceStr(data, s2[i], s1[i]);
      data = ReplaceStr(data,  "__", s2[i]);
    }
    data = HmsBase64Decode(data);
    HmsRegExMatch('^(.*%....)', data, data);
    data = HmsPercentDecode(data);
    string decoded="", digits="0123456789abcdefghijklmnopqrstuvwxyz"; // base36 decode
    for (i=1; i < Length(data); i+=2) {
      if (data[i]!="!") break; i++;
      decoded += chr((pos(data[i], digits)-1)*36 + pos(data[i+1], digits)-1);
    }
    data = decoded;
  }
  return data;
}

///////////////////////////////////////////////////////////////////////////////
// Получение ссылки с ресурса buildplayer
void GetLink_HLS(string sLink) {
  string sQual, sSelectedQual, html, js, data, sVal, path, sServ, hash, id;
  HmsRegExMatch("^(http.*?//[^/]+)", sLink, sServ);
  html = HmsUtf8Decode(HmsDownloadURL(sLink, 'Referer: '+mpFilePath+'\r\nOrigin: '+sServ));
  HmsRegExMatch('hlsList.*"\\d+":"(.*?)"', html, MediaResourceLink); // Первый вариант
  HmsRegExMatch('"(?:hls|file)":"(.*?)"' , html, MediaResourceLink); // Второй вариант
  if ((MediaResourceLink=="") && HmsRegExMatch('videoFilesEmdedCode[^>]+src=.?"(.*?)"', html, MediaResourceLink)) {
    if (LeftCopy(MediaResourceLink, 4)!='http') MediaResourceLink = 'http:'+Trim(MediaResourceLink);
    MediaResourceLink = HmsJsonDecode(MediaResourceLink);
    if (Pos("allohastream", MediaResourceLink)>0)
      GetLink_Allohastream(MediaResourceLink, "Referer: "+sLink);
    if (Pos("multikland", MediaResourceLink)>0) {
      sLink = MediaResourceLink; MediaResourceLink = "";
      GetLink_HLS(sLink);
    } else
      GetLink_tvmovies(MediaResourceLink, "Referer: "+sLink);
    return;
  }  
  MediaResourceLink = HmsJsonDecode(MediaResourceLink);
  if ((MediaResourceLink=="") && (Pos("bazon", sLink)>0)) {
    if (html!='') {
      if (HmsRegExMatch('<script>eval(\\(.*?\\))</script>', html, js, 1, PCRE_SINGLELINE)) data = jsEval(js);
      HmsRegExMatch('path:"(.*?)"', data, path);
      if (HmsRegExMatch("eval(\\(.*?split\\('\\|'\\),0,{}\\)\\))", data, js, 1, PCRE_SINGLELINE)) data = jsEval(js);
      HmsRegExMatch('file="(.*?)";', data, sVal);
      sVal = ReplaceStr(sVal, '"+"', "");
      MediaResourceLink = BazonDecode(sVal, path);
    } else {
      MediaResourceLink = sLink;
    }
  }
  if ((MediaResourceLink=="") && HmsRegExMatch("u-(cdn|play|\\w+)\\.", sLink, '')) {
    HmsRegExMatch('"hash"\\s*:\\s*"(.*?)"', html , hash);
    HmsRegExMatch('.*/(.*)'               , sLink, id  );
    HmsRegExMatch(  '"id"\\s*:\\s*"(.*?)"', html , id  );
    data = HmsUtf8Decode(HmsDownloadURL("https://ustore.bz/getContentJson.php?hash="+hash+"&id="+id, 'Referer: '+mpFilePath+'\r\nOrigin: '+sServ));
    HmsRegExMatch('"url"\\s*:\\s*\\[[^\\]]*"(.*?)"', data, sVal, 1, PCRE_SINGLELINE);
    MediaResourceLink = UstoreDecode(sVal);
    HmsRegExMatch('^(.*?\\.mp4):hls:manifest.m3u8', MediaResourceLink, MediaResourceLink);
  }
  if (LeftCopy(MediaResourceLink, 1)=='[') {
    TStringList QLIST = TStringList.Create();
    int n = WordCount(MediaResourceLink, ',');
    for (int i=1; i<=n; i++) {
      sLink = ExtractWord(i, MediaResourceLink, ',');
      HmsRegExMatch2('\\[(\\d+).*?\\](.*)', sLink, sQual, sLink);
      QLIST.Values[Format('%.4d', [StrToInt(sQual)])] = sLink;
    }
    QLIST.Sort();
    HmsRegExMatch('--quality=(\\w+)', mpPodcastParameters, sQual);
    if (QLIST.Count > 0) {
      if      (sQual=='low'   ) sSelectedQual = QLIST.Names[0];
      else if (sQual=='medium') sSelectedQual = QLIST.Names[Round((QLIST.Count-1) / 2)];
      else if (sQual=='high'  ) sSelectedQual = QLIST.Names[QLIST.Count - 1];
      else if (HmsRegExMatch('(\\d+)', sQual, sQual)) {
        extended minDiff = 999999; // search nearest quality
        for (i=0; i < QLIST.Count; i++) {
          extended diff = StrToInt(QLIST.Names[i]) - StrToInt(sQual);
          if (Abs(diff) < minDiff) {
            minDiff = Abs(diff);
            sSelectedQual = QLIST.Names[i];
          }
        }
      }
    }
    MediaResourceLink = QLIST.Values[sSelectedQual];
    QLIST.Free();
  }
  if (Pos("bazon", MediaResourceLink)>0) {
    if (Trim(mpComment)!='') mpFilePath = mpComment;
    MediaResourceLink = '-headers "Referer: '+mpFilePath+'" -user_agent "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/81.0.4044.138 Safari/537.36" -i "'+Trim(MediaResourceLink)+'"';
    return;
  }
  if (LeftCopy(MediaResourceLink, 1)=='/') MediaResourceLink = HmsExpandLink(MediaResourceLink, 'http:');
}

///////////////////////////////////////////////////////////////////////////////
// Получение ссылки с ресурса videoframe
void GetLink_videoframe(string sLink) {
  string html, data, post, token, type, name, headers, ret, id, season, episode;
  headers = "https://videoframe.at/\r\n"+
            "X-REF: hdkinoteatr.com\r\n"+
            "Origin: https://videoframe.at\r\n";
  html = HmsUtf8Decode(HmsDownloadURL(sLink, 'Referer: '+headers));
  HmsRegExMatch('data-token=[\'"](.*?)[\'"]', html, token);
  HmsRegExMatch('data-type=[\'"](.*?)[\'"]' , html, type );
  HmsRegExMatch('season\\s*=\\s*[\'"](\\d+)[\'"]' , html, season );
  HmsRegExMatch('episode\\s*=\\s*[\'"](\\d+)[\'"]', html, episode);
  post = 'token='+token+'&type='+type+'&season='+season+'&episode='+episode;
  data = HmsSendRequestEx('videoframe.at', '/loadvideo', 'POST', 'application/x-www-form-urlencoded', headers, post, 443, 16, ret, true);
  TJsonObject JSON = TJsonObject.Create();
  JSON.LoadFromString(data);
  MediaResourceLink = JSON.S['src'];
  JSON.Free;
}

///////////////////////////////////////////////////////////////////////////////
// Получение ссылки kodik.cc
void GetLink_Kodik(string sLink) {
  string sHtml, sData, sName, sHeight, sPost, sQual, sVal, sServ, sRet, d, d_sign, pd, pd_sign, ref, ref_sign, bad_user, hash2, type, hash, id;
  string sSelectedQual = '', sMsg; int iMinPriority = 99, iPriority; 
  
  int i; float f; TRegExpr RE; bool bQualLog; TJsonObject JSON, LINKS; TStringList QLIST;
  
  string sHeaders = sLink+'/\r\n'+
                    'User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:13.0) Gecko/20100101 Firefox/13.0\r\n';
  
  // Проверка установленных дополнительных параметров
  HmsRegExMatch('--quality=(\\w+)', mpPodcastParameters, sQual);
  bQualLog = Pos('--qualitylog'   , mpPodcastParameters) > 0;
  
  sHtml = HmsDownloadURL(sLink, 'Referer: '+sHeaders, true);
  HmsRegExMatch('iframe.src\\s*=\\s*"(.*?)"', sHtml, sLink);
  sHtml = HmsDownloadURL("http:"+sLink, 'Referer: '+sHeaders, true);
  
  HmsRegExMatch('\\sdomain\\s*=\\s*"(.*?)"'  , sHtml, d       );
  HmsRegExMatch('\\sd_sign\\s*=\\s*"(.*?)"'  , sHtml, d_sign  );
  HmsRegExMatch('\\spd\\s*=\\s*"(.*?)"'      , sHtml, pd      );
  HmsRegExMatch('\\spd_sign\\s*=\\s*"(.*?)"' , sHtml, pd_sign );
  HmsRegExMatch('\\sref\\s*=\\s*"(.*?)"'     , sHtml, ref     );
  HmsRegExMatch('\\sref_sign\\s*=\\s*"(.*?)"', sHtml, ref_sign);
  HmsRegExMatch('videoInfo.type\\s*=\\s*["\'](.*?)["\']', sHtml, type);
  HmsRegExMatch('videoInfo.hash\\s*=\\s*["\'](.*?)["\']', sHtml, hash);
  HmsRegExMatch('videoInfo.id\\s*=\\s*["\'](.*?)["\']'  , sHtml, id  );
  
  if (HmsRegExMatch('(/assets/js/app.promo[^"\']+)', sHtml, sVal)) {
    sData = HmsDownloadURL("http://kodik.info"+sVal, 'Referer: '+sHeaders, true);
    HmsRegExMatch('hash2:"(.*?)"', sData, hash2);
  } else {
    HmsLogMessage(2, mpTitle+': Ошибка получения ссылки на js, где прячится hash2.');
    return;
  }
  
  sPost = "d="+d+"&d_sign="+d_sign+"&pd="+pd+"&pd_sign="+pd_sign+"&ref="+HmsHttpEncode(ref)+"&ref_sign="+ref_sign+"&bad_user=false&hash2="+hash2+"&type="+type+"&hash="+hash+"&id="+id;
  sData = HmsSendRequestEx('kodik.cc', '/get-vid', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', sHeaders, sPost, 80, 16, sRet, true);
  
  JSON = TJsonObject.Create();
  QLIST = TStringList.Create();
  try {
    JSON.LoadFromString(sData);
    // Собираем список ссылок разного качества
    LINKS = JSON.O['links'];
    for (i = 0; i < LINKS.Count; i++) {
      sName = LINKS.Names[i];
      sLink = "http:"+LINKS.S[sName+'\\0\\src'];
      HmsRegExMatch('^(.*?\\.mp4):hls:manifest.m3u8', sLink, sLink);
      sHeight = Format('%.5d', [StrToInt(sName)]);
      QLIST.Values[sHeight] = sLink;
      MediaResourceLink = sLink; // по-умолчанию
      // Если установлен ключ --quality или в настройках подкаста выставлен приоритет выбора качества
      // ------------------------------------------------------------------------
      iPriority = HmsMediaFormatPriority(StrToInt(sHeight), mpPodcastMediaFormats);
      if ((iPriority >= 0) && (iPriority < iMinPriority)) {
        iMinPriority  = iPriority;
        sSelectedQual = sHeight;
      }
    }
    QLIST.Sort();
    if (QLIST.Count > 0) {
      if      (sQual=='low'   ) sSelectedQual = QLIST.Names[0];
      else if (sQual=='medium') sSelectedQual = QLIST.Names[Round((QLIST.Count-1) / 2)];
      else if (sQual=='high'  ) sSelectedQual = QLIST.Names[QLIST.Count - 1];
      else if (HmsRegExMatch('(\\d+)', sQual, sQual)) {
        extended minDiff = 999999; // search nearest quality
        for (i=0; i < QLIST.Count; i++) {
          extended diff = StrToInt(QLIST.Names[i]) - StrToInt(sQual);
          if (Abs(diff) < minDiff) {
            minDiff = Abs(diff);
            sSelectedQual = QLIST.Names[i];
          }
        }
      }
    }
    if (sSelectedQual != '') MediaResourceLink = QLIST.Values[sSelectedQual];
    if (bQualLog) {
      sMsg = 'Доступное качество: ';
      for (i = 0; i < QLIST.Count; i++) {
        if (i>0) sMsg += ', ';
        sMsg += IntToStr(StrToInt(QLIST.Names[i])); // Обрезаем лидирующие нули
      }
      if (sSelectedQual != '') sSelectedQual = IntToStr(StrToInt(sSelectedQual));
      else sSelectedQual = 'Auto';
      sMsg += '. Выбрано: ' + sSelectedQual;
      HmsLogMessage(1, mpTitle+'. '+sMsg);
    }
    
  } finally { JSON.Free; QLIST.Free; }
  
  if (LeftCopy(Trim(MediaResourceLink), 2)=='//')
    MediaResourceLink = 'http:'+Trim(MediaResourceLink);
  
  if (ExtractFileExt(MediaResourceLink)=='.mp4') {
    sLink = MediaResourceLink+':hls:manifest.m3u8';
    // Получение длительности видео, если она не установлена
    // ------------------------------------------------------------------------
    sVal = Trim(PodcastItem.ItemOrigin[mpiTimeLength]);
    if ((sVal=='') || (RightCopy(sVal, 6)=='00.000') || (RightCopy(sVal, 5)=='00:00') && (ExtractFileExt(sLink)==".m3u8")) {
      sHtml = HmsDownloadUrl(sLink, 'Referer: '+sHeaders, true);
      RE = TRegExpr.Create('#EXTINF:(\\d+.\\d+)', PCRE_SINGLELINE); f=0;
      if (RE.Search(sHtml)) do f += StrToFloatDef(RE.Match(1), 0); while (RE.SearchAgain());
      RE.Free;
      if (f > 0) PodcastItem.ItemOrigin[mpiTimeLength] = HmsTimeFormat(Round(f))+'.000';
    }
    // ------------------------------------------------------------------------
  }
}

///////////////////////////////////////////////////////////////////////////////
// Javascript Eval
string jsEval(string sData) {
  Variant objScript; string sResult = '';
  
  try { objScript = CreateOleObject('MSScriptControl.ScriptControl'); } except { };
  if (VarType(objScript) != varDispatch) { 
    HmsLogMessage(2, 'Не могу создать ActiveXObject MSScriptControl.ScriptControl'); 
    return ''; 
  }
  objScript.Language = 'JavaScript';
  try { sResult = objScript.Eval(sData); } except { };
  return sResult;
}

///////////////////////////////////////////////////////////////////////////////
// Проверка ссылки на известные нам ресурсы видео и получение ссылки на поток
void GetLink() {
  if      (Pos('videoframe'   , mpFilePath)>0) GetLink_Videoframe  (mpFilePath);
  else if (Pos("alloha"       , mpFilePath)>0) GetLink_Allohastream(mpFilePath);
  else if (Pos('tvmovies.in'  , mpFilePath)>0) GetLink_tvmovies    (mpFilePath);
  else if (Pos('cdn.tv'       , mpFilePath)>0) GetLink_tvmovies    (mpFilePath);
  else if (Pos('videocdn'     , mpFilePath)>0) GetLink_tvmovies    (mpFilePath);
  else if (Pos('buildplayer'  , mpFilePath)>0) GetLink_HLS         (mpFilePath);
  else if (Pos('farsihd'      , mpFilePath)>0) GetLink_HLS         (mpFilePath);
  else if (Pos('kodik'        , mpFilePath)>0) GetLink_Kodik       (mpFilePath);
  else if (Pos('bazon'        , mpFilePath)>0) GetLink_HLS         (mpFilePath);
  else if (Pos('multikland'   , mpFilePath)>0) GetLink_HLS         (mpFilePath);
  else if (HmsRegExMatch('pleer\\w{2}\\.', mpFilePath, '')) GetLink_HLS(mpFilePath);
  else if (HmsRegExMatch('//vid\\d+'     , mpFilePath, '')) GetLink_HLS(mpFilePath);
  else if (HmsRegExMatch('(youtube.com|youto.be)', mpFilePath, '')) GetLink_YouTube33(mpFilePath);
  else if (LeftCopy(mpFilePath, 4)=='Info') VideoPreview();
  else if (LeftCopy(mpFilePath, 4)=='-Fav') AddRemoveFavorites();
  else if ((HmsFileMediaType(mpFilePath)==mtVideo) || (HmsFileMediaType(mpFilePath)==7)) MediaResourceLink = mpFilePath;
  else GetLink_HLS(mpFilePath);
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
        Item.CopyProperties(Src, [mpiTitle, mpiThumbnail, mpiJsonInfo, mpiKPID]);
      }
    }
  }
  MediaResourceLink = ProgramPath()+'\\Presentation\\Images\\videook.mp4';
  if (bExist) { mpTitle = "Добавить в избранное" ; mpFilePath = "-FavDel="+sLink; }
  else        { mpTitle = "Удалить из избранного"; mpFilePath = "-FavDel="+sLink; }
  PodcastItem[mpiTitle] = mpTitle;
  PodcastItem.ItemOrigin.ItemParent[mpiTitle] = mpTitle;
  PodcastItem[mpiFilePath] = mpFilePath;
  PodcastItem.ItemOrigin.ItemParent[mpiFilePath] = mpFilePath;
  HmsIncSystemUpdateID(); // Говорим устройству об обновлении содержания
}

///////////////////////////////////////////////////////////////////////////////
// Пометка просмотренной серии
void MarkViewed() {
  string sMark, sSeason, sEpisode; THmsScriptMediaItem Item = PodcastItem;
  // Если это не серия сериала - выходим
  if (!HmsRegExMatch2('season=(\\d+).*?episode=(\\d+)', mpFilePath, sSeason, sEpisode)) return;
  // Поиск родительской папки сериала (где есть информация о сериала в mpiJsonInfo)
  while ((Trim(Item[mpiJsonInfo])=="") && (Item.ItemParent!=nil)) Item=Item.ItemParent;
  if (Trim(Item[mpiJsonInfo])=="") return; // Если не найдена - выходим
  sMark = 's'+sSeason+'e'+sEpisode+';';    // Номер сезона и серии
  if (Pos(sMark, Item[mpiComment]) < 1) Item[mpiComment] += sMark; // Добавляем в комментарий
  ItemMarkViewed(PodcastItem);
  HmsDatabaseAutoSave(true);
}

///////////////////////////////////////////////////////////////////////////////
//                     Г Л А В Н А Я   П Р О Ц Е Д У Р А                     //
///////////////////////////////////////////////////////////////////////////////
{
  if (PodcastItem.IsFolder) {
    // Зашли в папку подкаста - создаём ссылки внутри неё
    if (HmsRegExMatch('kinopoisk.ru/film/\\d+/video', mpFilePath, ''))
      CreateTrailersLinks(mpFilePath);  // Это папка "Трейлеры"
    else
      CreateLinks();
  
  } else { 
    // Запустили фильм - получаем ссылку на медиа-поток
    GetLink();  
    
    if ((Trim(MediaResourceLink)!='') && (Pos('--markonplay', mpPodcastParameters)>0)) 
      MarkViewed();
    
    if (Pos('.m3u8', MediaResourceLink)>0) MediaResourceLink = ' '+Trim(MediaResourceLink);
  } 

}
