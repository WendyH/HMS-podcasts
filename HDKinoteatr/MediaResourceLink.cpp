// 2018.10.05
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

// -------------------------------------------- Получение ссылки на vk.com ----
bool GetLink_VK(string sLink) {
  string sHtml, sVal, host, uid, vkid, vtag, max_hd, no_flv, res, extra, cat;
  string ResolutionList='0:240, 1:360, 2:480, 3:720', sQAval, sQSel, sID;
  int i, iPriority=0, iMinPriority=99; int nFlag=0; 
  
  sHtml = HmsDownloadURL(sLink, 'Referer: '+sLink, true);
  
  if ((sHtml=="") || !HmsRegExMatch('vtag["\':=\\s]+([0-9a-z]+)', sHtml, vtag)) {
    //LogInSiteVK();
    if (HmsRegExMatch2("oid=(.*?)&.*?id=(.*?)&", sLink, sVal, sID))
      sLink = "https://vk.com/video"+sVal+"_"+sID;
    sHtml = HmsDownloadUrl(sLink, 'Referer: '+sLink, true);
  }
  
  sHtml = ReplaceStr(sHtml, '\\', '');
  host = ''; max_hd = '2';
  
  sLink = '';
  HmsRegExMatch('--quality=(\\d+)', mpPodcastParameters, sQSel);
  if (sQSel!='') HmsRegExMatch('"url'+sQSel+'":"(.*?)"', sHtml, sLink);
  if (sLink=='') HmsRegExMatch('"url720":"(.*?)"', sHtml, sLink);
  if (sLink=='') HmsRegExMatch('"url480":"(.*?)"', sHtml, sLink);
  if (sLink=='') HmsRegExMatch('"url360":"(.*?)"', sHtml, sLink);
  if (sLink=='') HmsRegExMatch('"url240":"(.*?)"', sHtml, sLink);
  if (sLink!='') {
    MediaResourceLink = HmsJsonDecode(sLink);
    return;
  }
  
  if (!HmsRegExMatch('vtag["\':=\\s]+([0-9a-z]+)', sHtml, vtag)) {
    if (HmsRegExMatch('(<div[^>]+video_ext_msg.*?</div>)', sHtml, sLink) ||
    HmsRegExMatch('<div style="position:absolute; top:50%; text-align:center; right:0pt; left:0pt;.*?>(.*?)</div>', sHtml, sLink)) {
      HmsLogMessage(2, PodcastItem.ItemOrigin.ItemParent[mpiTitle]+': vk.com сообщает - '+HmsHtmlToText(sLink));
      if (random > 0.5) MediaResourceLink = 'http://wonky.lostcut.net/vids/enotallow.avi';
      else VideoMessage('VK.COM СООБЩАЕТ:\n\n'+HmsHtmlToText(sLink));
    } else {
      HmsLogMessage(2, mpTitle+': не удалось обработать ссылку на vk.com');
      MediaResourceLink = 'http://wonky.lostcut.net/vids/error_getlink.avi';
    }
    return true;
  }
  HmsRegExMatch('[^a-z]host[=:"\'\\s]+(.*?)["\'&;,]', sHtml, host  );
  HmsRegExMatch('[^a-z]uid[=:"\'\\s]+([0-9]+)'      , sHtml, uid   );
  HmsRegExMatch('no_flv.*?(\\d)'                    , sHtml, no_flv);
  HmsRegExMatch('(?>hd":"|hd=|video_max_hd.*?)(\\d)', sHtml, max_hd);
  HmsRegExMatch('[^a-z]vkid[=:"\'\\s]+([0-9]+)'     , sHtml, vkid  );
  HmsRegExMatch('extra=([\\w-]+)'                   , sHtml, extra );
  HmsRegExMatch('/(\\d+)/u\\d+/'                    , sHtml, cat   );
  
  HmsRegExMatch(max_hd+':(\\d+)',            ResolutionList, res   );
  
  sQAval = 'Доступное качество: '; sQSel = '';
  
  // Если включен приоритет форматов, то ищем ссылку на более приоритетное качество
  if (gbQualityLog || (mpPodcastMediaFormats!='')) for (i=StrToIntDef(max_hd, 3); i>=0; i--) {
    HmsRegExMatch(IntToStr(i)+':(\\d+)', ResolutionList, sVal);
    sQAval += sVal + '  ';
    if (sQSel != '') {
      if (StrToIntDef(res, 0)>StrToIntDef(sQSel, 0)) res = sVal;
    } else if (mpPodcastMediaFormats != '') {
      iPriority = HmsMediaFormatPriority(StrToIntDef(sVal, 0), mpPodcastMediaFormats);
      if ((iPriority>=0)&&(iPriority<iMinPriority)) {iMinPriority = iPriority; res=sVal;}
    }
  }
  if (gbQualityLog) HmsLogMessage(1, mpTitle+': '+sQAval+'Выбрано: '+res);
  
  if (LeftCopy(uid, 1)!='u') uid = 'u' + Trim(uid);
  if (Trim(host)=='') HmsRegExMatch('ajax.preload.*?<img[^>]+src="(http://.*?/)', sHtml, host);
  
  if (uid=='0') MediaResourceLink = host+'assets/videos/'+vtag+''+vkid+'.vk.flv';
  else          MediaResourceLink = host + uid+'/videos/'+vtag+'.'+res+'.mp4';
  if (Trim(extra)!='') MediaResourceLink += '?extra='+extra;
  if (Trim(cat  )!='') MediaResourceLink = ReplaceStr(MediaResourceLink, '/'+uid, '/'+cat+'/'+uid);
  
  HmsRegExMatch(";url"+res+"=(.*?)&", sHtml, MediaResourceLink);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Получение ссылки с moonwalk.cc
void GetLink_Moonwalk(string sLink) {
  string sHtml, sData,sLinks, sJsData, sPost, sVer, sQual, sVal, sServ, sVar, sUrlBase, sJSONParams;
  int i, n; float f; TRegExpr RE; bool bHdsDump, bQualLog;
  TJsonObject JSON, OPTIONS;

  string sUserAgent = 'Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36';
  string sHeaders = sLink+'\r\n'+
                    'user-agent: '+sUserAgent+'\r\n';
  // Для Windows XP - отключаем сжатие страниц. Ибо автоматом ответы не распаковываются.
  if (StrToFloatDef(RegistryRead("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\CurrentVersion"), 0) < 6) {
    sHeaders += 'accept-encoding: identity\r\n';
  } else {
    sHeaders += 'accept-encoding: gzip, deflate, br\r\n';
  }

  // Проверка установленных дополнительных параметров
  HmsRegExMatch('--quality=(\\w+)', mpPodcastParameters, sQual);
  bQualLog = Pos('--qualitylog'   , mpPodcastParameters) > 0;

  // Замена домена moonwalk.co и moonwalk.pw на moonwalk.cc
  HmsRegExReplace('(.*?moonwalk.)(co|pw)(.*)', sLink, '$1cc$3', sLink);
  sHtml = HmsDownloadURL(sLink, 'Referer: '+sHeaders);

  if (HmsRegExMatch('<iframe[^>]+src="(http.*?)"', sHtml, sLink)) {
    // Если внутри есть ссылка на iframe - загружаем его
    HmsRegExReplace('(.*?moonwalk.)(co|pw)(.*)', sLink, '$1cc$3', sLink);
    sHtml = HmsDownloadURL(sLink, 'Referer: '+sHeaders);
  }
  HmsRegExMatch2('(.*?//([^/]+))', sLink, sUrlBase, sServ); // Получаем UrlBase и домен
  sHeaders += 'Origin: '+sUrlBase+'\r\n'+
              'X-Requested-With: XMLHttpRequest\r\n'+
              ':authority: '+sServ+'\r\n';

  JSON     = TJsonObject.Create();
  OPTIONS  = TJsonObject.Create();
  try {

    // Ищем значения параметров (this.options)
    if (!HmsRegExMatch('VideoBalancer\\((.*?)\\);', sHtml, sData)) {
      if (HmsRegExMatch3("//.*?/(.*?)/(.*?)/[^?]+\\??(.*)", sLink, sVer, sVal, sData))
        sHtml = HmsDownloadURL("http://api.lostcut.net/?action=moonhtml&t="+sVer+"&i="+sVal+"&p="+HmsHttpEncode(sData), 'Referer: '+sHeaders);
      if (!HmsRegExMatch('VideoBalancer\\((.*?)\\);', sHtml, sData)) {
        sVal = "Не найдены данные VideoBalancer в iframe.";
        if (HmsRegExMatch("<div[^>]+absolute.*?</div>", sHtml, sVal, 0, PCRE_SINGLELINE)) sVal = HmsRemoveLineBreaks(HmsHtmlToText(sVal, 65001));
        HmsLogMessage(2, mpTitle+": "+sVal);
        // Заглушка
        sVal  = ExtractWord(Round(Random*2)+1, "wZ5JJ1CRWdO0,IRcEgNmu9AUn,ebxWWiOq6VbO", ",");
        sData = HmsDownloadURL("https://studio.stupeflix.com/v/"+sVal+"/", sHeaders, true);
        HmsRegExMatch('"(http[^>"]+\\.mp4)', sData, MediaResourceLink);
        return;
      }
    }

    OPTIONS.LoadFromString(sData);

    // Если есть субтитры и их ещё локально не сохраняли - загружаем
    if ((OPTIONS.S["subtitles\\master_vtt"]!="") && (!FileExists(PodcastItem[mpiSubtitleLanguage]))) {
      sData = HmsUtf8Decode(HmsDownloadURL(OPTIONS.S["subtitles\\master_vtt"], "Referer: "+sHeaders));
      RE = TRegExpr.Create('(\\d{2}:\\d{2}:[\\d:,.\\s->]+.*?)(?=\n\\d{2}:d{2}:|\n\\d+\n|$)', PCRE_SINGLELINE);
      i = 1; sVal = ""; // Форматируем
      if (RE.Search(sData)) do { sVal += Format("%d\r\n%s\r\n\r\n", [i, RE.Match]); i++; } while (RE.SearchAgain);
      sLink = HmsSubtitlesDirectory+'\\'+PodcastItem.ItemID+'.srt';
      HmsStringToFile(sVal, sLink);             // Сохраняем субтитры локально
      PodcastItem[mpiSubtitleLanguage] = sLink; // Указываем новое расположение
    }

    // Получение ссылки на js-скрипт, где есть список параметров POST запроса
    if (!HmsRegExMatch('src="([^"]+/video-[^"]+.js)', sHtml, sVal)) {
      HmsLogMessage(2, mpTitle+": Не найдена ссылка на js-скрипт в iframe.");
      return;
    }
    sLinks = HmsExpandLink(sVal, sUrlBase);
    sJsData = HmsDownloadURL(sLinks, 'Referer: '+sHeaders);
    // Получаем параметры POST запроса
    if (!HmsRegExMatch('getVideoManifests.*?(\\{.*?\\})', sJsData, sData)) {
      HmsLogMessage(2, mpTitle+": Не найдена функция getVideoManifests в скрипте плеера moonwalk.");
      return;
    }
    if (!HmsRegExMatch('\\D=(\\{.*?\\})', sData, sJSONParams)) {
      HmsLogMessage(2, mpTitle+": Не найдены параметры для POST запроса в функции getVideoManifests.");
      return;
    }

    // Получаем данные для шифрования
    string sKey='', iv='', a1, a2, a3, a4, a5;
    HmsRegExMatch(',r=\\[(".*?)\\]', sJsData, sVal);
    sVal = ReplaceStr(sVal, '"', '');
    
    HmsRegExMatch('0x0"\\)]="(.*?)"', sJsData, a1);
    HmsRegExMatch('e2a9"\\]="(.*?)"', sJsData, a2);
    HmsRegExMatch('0x5"\\)]="(.*?)"', sJsData, a3);
    HmsRegExMatch('0xb"\\)]="(.*?)"', sJsData, a4);
    HmsRegExMatch('0xf"\\)]="(.*?)"', sJsData, a5);
    
    try {
      sKey = a1+a2+a3+ExtractWord(14, sVal, ',')+a4+a5+ExtractWord(24, sVal, ',');
      iv   = ExtractWord(27, sVal, ',');
    } finally {}
    
    if (Length(sKey)!=64) { HmsLogMessage(2, mpTitle+': encryption key not found.'); return; }
    if (iv=='')           { HmsLogMessage(2, mpTitle+': encryption  iv not found.'); return; }

    string sData4Encrypt = '{"a":'+OPTIONS.S['partner_id']+',"b":'+OPTIONS.S['domain_id']+',"c":false,"e":"'+OPTIONS.S['video_token']+'","f":"'+sUserAgent+'"}';
    int padding = 16 - (Length(sData4Encrypt) % 16);
    for (i=0; i < padding; i++) sData4Encrypt += chr(padding); // PKCS7 Padding
    sVal  = HmsCryptCipherEncode("Rijndael", sData4Encrypt, HmsHexToString(sKey), HmsHexToString(iv), cmCBCx, "MIME64");
    sPost = "q="+HmsHttpEncode(sVal);
    
    sData = HmsSendRequest(sServ, "/vs", 'POST', 'application/x-www-form-urlencoded; Charset=UTF-8', sHeaders, sPost, 80, true);
    sVar  = "";
    JSON.LoadFromString(sData);
    bool bMP4 = Pos("--mp4", mpPodcastParameters)>0;
    if (bMP4 && (JSON.B["mp4" ])) sVar = "mp4" ;
    else if     (JSON.B["m3u8"])  sVar = "m3u8";
    sLink = JSON.S[sVar];
    if (sLink!="") sData = HmsDownloadURL(sLink, "Referer: "+sHeaders, true);

  } finally { JSON.Free; OPTIONS.Free; }

  TStringList QLIST = TStringList.Create();

  if (sVar=="m3u8") {
    MediaResourceLink = sLink;
    // Получение длительности видео, если она не установлена
    // ------------------------------------------------------------------------
    sVal = Trim(PodcastItem.ItemOrigin[mpiTimeLength]);
    if ((sVal=='') || (RightCopy(sVal, 6)=='00.000')) {
      if (HmsRegExMatch('(http.*?)[\r\n$]', sData, sLink)) {
        sHtml = HmsDownloadUrl(sLink, 'Referer: '+sHeaders, true);
        RE = TRegExpr.Create('#EXTINF:(\\d+.\\d+)', PCRE_SINGLELINE); f=0;
        if (RE.Search(sHtml)) do f += StrToFloatDef(RE.Match(1), 0); while (RE.SearchAgain());
        RE.Free;
        if (f > 0) PodcastItem.ItemOrigin[mpiTimeLength] = HmsTimeFormat(Round(f))+'.000';
      }
    }
    // ------------------------------------------------------------------------

    // Если установлен ключ --quality или в настройках подкаста выставлен приоритет выбора качества
    // ------------------------------------------------------------------------
    string sSelectedQual = '', sMsg, sHeight; int iMinPriority = 99, iPriority;
    if ((sQual!='') || (mpPodcastMediaFormats!='')) {
      // Собираем список ссылок разного качества
      RE = TRegExpr.Create('#EXT-X-STREAM-INF:RESOLUTION=\\d+x(\\d+).*?[\r\n](http.+)$', PCRE_MULTILINE);
      if (RE.Search(sData)) do {
        sHeight = Format('%.5d', [StrToInt(RE.Match(1))]);
        sLink   = RE.Match(2);
        QLIST.Values[sHeight] = sLink;
        iPriority = HmsMediaFormatPriority(StrToInt(sHeight), mpPodcastMediaFormats);
        if ((iPriority >= 0) && (iPriority < iMinPriority)) {
          iMinPriority  = iPriority;
          sSelectedQual = sHeight;
        }
      } while (RE.SearchAgain());
      RE.Free;
    }
    // ------------------------------------------------------------------------

  } else if (sVar=="mp4") {
    // Составляем список ссылок с разным качеством
    JSON = TJsonObject.Create();
    try {
      JSON.LoadFromString(sData);
      for (i=0; i<JSON.Count; i++) {
        sVer    = JSON.Names[i];
        sLink   = JSON.S[sVer];
        sHeight = Format('%.5d', [StrToIntDef(sVer, 0)]);
        QLIST.Values[sHeight] = sLink;
      }
    } finally { JSON.Free(); }

  } else {
    HmsLogMessage(2, mpTitle+': Ошибка получения данных от сервера moonwalk.');
    if (DEBUG==1) {
      if (ServiceMode) sVal = SpecialFolderPath(0x37); // Общая папака "Видео"
        else           sVal = SpecialFolderPath(0);    // Рабочий стол
        sVal = IncludeTrailingBackslash(sVal)+'Moonwalk.log';
      HmsStringToFile(sHeaders+'\r\nsHtml:\r\n'+sHtml+'\r\nsPost:\r\n'+sPost+'\r\nsData:\r\n'+sData, sVal);
      HmsLogMessage(1, mpTitle+': Создан лог файл '+sVal);
    }
  }
  // Если сформирован список ссылок для разного качества
  if (QLIST.Count > 0) {
    QLIST.Sort();
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
  QLIST.Free;
  if (sVar=="m3u8") MediaResourceLink = " "+Trim(MediaResourceLink);
} // Конец функции поулчения ссылки с moonwalk.cc

///////////////////////////////////////////////////////////////////////////////
// Формирование ссылки для воспроизведения через HDSDump.exe
void HDSLink(string sLink, string sQual = '') {
  string sData, sExePath, sVal;
  sExePath = ProgramPath+'\\Transcoders\\hdsdump.exe';
  MediaResourceLink = Format('cmd://"%s" --manifest "%s" --outfile "<OUTPUT FILE>"', [sExePath, sLink]);
  if (sQual       != '') MediaResourceLink += ' --quality ' + sQual;
  if (mpTimeStart != '') MediaResourceLink += ' --skip '    + mpTimeStart;
  PodcastItem[mpiTimeSeekDisabled] = true;
  // Получение длительности видео, если она не установлена
  if ((Trim(PodcastItem[mpiTimeLength])=='') || (RightCopy(PodcastItem[mpiTimeLength], 6)=='00.000')) {
    sData = HmsDownloadUrl(sLink, 'Referer: '+sLink, true);
    if (HmsRegExMatch('<duration>(\\d+)', sData, sVal))
      PodcastItem.Properties[mpiTimeLength] = StrToInt(sVal);
  }
}


///////////////////////////////////////////////////////////////////////////////
// Получение ссылки на Youtube
bool GetLink_Youtube31(string sLink) {
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
  
  if (sVideoID=='') return VideoMessage('Невозможно получить Video ID в ссылке Youtube');
  
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
      VideoMessage(sMsg); 
      return; 
    } else {
      sData = HmsJsonDecode(sData);
      HmsRegExMatch('"url":"(.*?)"', sData, MediaResourceLink);
      return true;
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
      VideoMessage(sMsg); 
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
  if (HmsRegExMatch('(?:/embed/|v=)([\\w-_]+)', sLink, sImg))
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
  sLink = HmsSendRequestEx('wonky.lostcut.net', '/videopreview.php?p='+gsPreviewPrefix, 'POST', 
  'application/x-www-form-urlencoded', '', sData, 80, 0, '', true);
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
// Создание списка серий сериала с Moonwalk.cc
void CreateMoonwallkLinks(string sLink, string sKPID='') {
  string sData; TJsonObject PLAYLIST;
  // Пытаемся получить ID кинопоиска
  if (sKPID=='') {
    sKPID = Trim(PodcastItem[mpiKPID]); // Получаем запомненный kinopoisk ID
    if (sKPID=='') HmsRegExMatch('/images/(film|film_big)/(\\d+)', mpThumbnail, sKPID, 2);
    if (sKPID=='') HmsRegExMatch('/iphone360_(\\d+)', mpThumbnail, sKPID);
  }
  
  sData = HmsDownloadURL(gsAPIUrl+'series?url='+HmsHttpEncode(sLink)+'&kpid='+sKPID, "Accept-Encoding: gzip, deflate", true);
  sData = HmsUtf8Decode(sData);
  PLAYLIST = TJsonObject.Create();
  try {
    PLAYLIST.LoadFromString(sData);
    CreateVideosFromJsonPlaylist(PLAYLIST.AsArray(), PodcastItem);
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
      sLink = ReplaceStr(sLink, "moon.hdkinoteatr.com", "moonwalk.cc");

      if (sName=='Фильм') sName = mpTitle;
      if ((Pos('/serial/', sLink)>0) && (Pos('episode', sLink)<1))
        CreateFolder(Folder, sName, sLink, sImg);
      
      else if (Pos('youtu', sLink) > 0) {
        
        if (Pos('Трейлер', sName)>0)
          CreateMediaItem(Folder, sName, sLink, sImg, 230);
        else
          CreateMediaItem(Folder, sName, sLink, sImg, gnDefaultTime);
      } else {
        // Форматируем номер в два знака
        if (sName!=mpTitle) {
          if (HmsRegExMatch("^(\\d+)", sName, sVal)) 
            sName = ReplaceStr(sName, sVal, Trim(Format(sFmt, [StrToInt(sVal), ""])));
          else
            sName = Format(sFmt, [i+1, sName]);
        }
        Item = CreateMediaItem(Folder, sName, sLink, sImg, gnDefaultTime);
        if (JOBJECT.B["subs"]) Item[mpiSubtitleLanguage] = HmsSubtitlesDirectory+'\\'+Item.ItemID+'.srt';
        FillVideoInfo(Item);
      }
    }
  }
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
// Создание ссылок на фильм или сериал
void CreateLinks() {
  string sData, sName, sLink, sVal; TJsonObject VIDEO, PLAYLIST; THmsScriptMediaItem Item;
  VIDEO    = TJsonObject.Create();
  PLAYLIST = TJsonObject.Create();
  
  bool bTrailers = Pos('--trailersfolder', mpPodcastParameters) > 0;
  
  try {
    VIDEO.LoadFromString(PodcastItem[mpiJsonInfo]);
    
    gStart = ConvertToDate(VIDEO.S['date']); // Устанавливаем началную точку для даты ссылок
    gnDefaultTime = VIDEO.I['time'];
    sLink = VIDEO.S['link'];
    sName = VIDEO.S['name'];
    sLink = ReplaceStr(sLink, "moon.hdkinoteatr.com", "moonwalk.cc");
    
    if (HmsRegExMatch('(\\[|\\{)', sLink, '')) {
      sLink = ReplaceStr(sLink, "episode=", "e="); // Если это ссылки на сериалы, то создавать папки
      PLAYLIST.LoadFromString(sLink);
      CreateVideosFromJsonPlaylist(PLAYLIST.AsArray(), PodcastItem);
    } else {
      
      if (HmsRegExMatch('/serial/', sLink, '')) {
        CreateMoonwallkLinks(sLink, VIDEO.S['kpid']); 
      } else {
        Item = CreateMediaItem(PodcastItem, sName, sLink, mpThumbnail, gnDefaultTime);
        FillVideoInfo(Item);
      }

    }

    // Создание папки "Трейлеры"
    if (bTrailers && VIDEO.B['kpid'] && (Pos('Трейлер', sLink) < 1))
      CreateFolder(PodcastItem, 'Трейлеры', 'http://www.kinopoisk.ru/film/'+VIDEO.S['kpid']+'/video/');
      
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
    
  } finally { VIDEO.Free; PLAYLIST.Free; }
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылок на фильм или сериал
void SearchAndPlayVideoByKPid() {
  string sData, sID;
  
  if (Trim(PodcastItem[mpiJsonInfo])!='') return;
  
  if (!HmsRegExMatch('kpid=(\\d+)', mpFilePath, sID)) return;
  sData = HmsDownloadURL(gsAPIUrl+'videos?kpid='+sID, "Accept-Encoding: gzip, deflate", true);
  HmsRegExMatch('^\\[(.*)\\]', sData, sData, 1, PCRE_SINGLELINE);
  PodcastItem[mpiJsonInfo] = HmsUtf8Decode(sData);
}

///////////////////////////////////////////////////////////////////////////////
// Проверка ссылки на известные нам ресурсы видео и получение ссылки на поток
void GetLink() {
  mpFilePath = ReplaceStr(mpFilePath, "moon.hdkinoteatr.com", "moonwalk.cc");
  if (HmsRegExMatch('(vk.com|vkontakte.ru)', mpFilePath, '')) {
    GetLink_VK(mpFilePath);

  } else if (HmsRegExMatch('(54.228.189.108|moonwalk.cc)', mpFilePath, '')) {
    GetLink_Moonwalk(mpFilePath);

  } else if (HmsRegExMatch('/(serial|video)/(.*?)/iframe', mpFilePath, '')) {
    GetLink_Moonwalk(mpFilePath);

  } else if (HmsRegExMatch('(youtube.com|youto.be)', mpFilePath, '')) {
    GetLink_YouTube31(mpFilePath);

  } else if (HmsFileMediaType(mpFilePath)==mtVideo) {
    MediaResourceLink = mpFilePath;

  } else if (LeftCopy(mpFilePath, 4)=='Info') {
    VideoPreview();
  
  } else if (LeftCopy(mpFilePath, 4)=='kpid') {
    SearchAndPlayVideoByKPid();
    
  } else if (LeftCopy(mpFilePath, 4)=='-Fav') 
    AddRemoveFavorites();
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
{
  if (PodcastItem.IsFolder) {
    // Зашли в папку подкаста - создаём ссылки внутри неё
    if (HmsRegExMatch('kinopoisk.ru/film/\\d+/video', mpFilePath, ''))
      CreateTrailersLinks(mpFilePath);  // Это папка "Трейлеры"
    else if (HmsRegExMatch('/serial/', mpFilePath, ''))
      CreateMoonwallkLinks(mpFilePath); // Зашли в вариант перевода сериала
    else
      CreateLinks();
  
  } else { 
    // Запустили фильм - получаем ссылку на медиа-поток
    GetLink();  
    if ((Trim(MediaResourceLink)=='') && HmsRegExMatch('"link":.*?(http[^"\']+)', PodcastItem[mpiJsonInfo], mpFilePath)) {
      mpFilePath = HmsJsonDecode(mpFilePath);
      GetLink();  
    }
    
    if ((Trim(MediaResourceLink)!='') && (Pos('--markonplay', mpPodcastParameters)>0)) 
      MarkViewed();
  } 

}
