///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //

int    LOG = 0; // Вывод сообшений в лог окно для замера производительности
string gsUrlBase="http://tree.tv";
int    gnTotalItems=0; TDateTime gStart = Now; string sTime="01:40:00.000";
string gsCacheDir = IncludeTrailingBackslash(HmsTempDirectory)+'tree.tv';
string gsPreviewPrefix = "treetv";
Variant playerKeyParams = [1, 2, 293]; // key, g, p

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки на видео
THmsScriptMediaItem CreateMediaItem(THmsScriptMediaItem Folder, string sTitle, string sLink, string sImg='') {
  if (sTime=='') sTime = '01:40:00.000';
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, Folder.ItemID);
  Item[mpiTitle     ] = sTitle;
  Item[mpiThumbnail ] = sImg;
  Item[mpiCreateDate] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0));
  Item[mpiTimeLength] = sTime;
  gnTotalItems++;
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки-ошибки
void CreateErrorItem(string sMsg) {
  THmsScriptMediaItem Item = HmsCreateMediaItem('Err'+IntToStr(PodcastItem.ChildCount), PodcastItem.ItemID);
  Item[mpiTitle     ] = sMsg;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/icons/symbol-error.png';
}

///////////////////////////////////////////////////////////////////////////////
// Добавление информационных элементов
void AddInfoItems(char sHtml, TStrings Info) {
  char sData = sHtml;
  CreateInfoItem('КП'      , Info.Values["Kinopoisk"]);
  CreateInfoItem('IMDb'    , Info.Values["IMDb"     ]);
  CreateInfoItem('Год'     , Info.Values["Year"     ]);
  CreateInfoItem('Режиссер', Info.Values["Director" ]);
  CreateInfoItem('Жанр'    , Info.Values["Genre"    ]);
  CreateInfoItem('Страна'  , Info.Values["Country"  ]);
  CreateInfoItem('В ролях' , Info.Values["Actors"   ]);
  CreateInfoItem('Качество', Info.Values["Quality" ]);
}

///////////////////////////////////////////////////////////////////////////////
// Создание информационной ссылки
void CreateInfoItem(char sTitle, char sVal) {
  THmsScriptMediaItem Item; sVal = Trim(sVal);
  if (sVal=="") return;
  Item = CreateMediaItem(PodcastItem, sTitle+': '+sVal, 'Info'+IntToStr(PodcastItem.ChildCount));
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/vids/info.jpg';
  Item[mpiTimeLength] = '00:00:07.000';
}

// ----------------------------------------- Получение информации о фильме ----
TStrings GetInfoAboutFilm(string sHtml) {
  string sVal; TStrings Info = TStringList.Create();
  sHtml = StripHtmlComments(sHtml);
  SetInfo(Info, sHtml, "Title"   , "(<h1.*?</h1>)"); 
  SetInfo(Info, sHtml, "Year"    , "Год:.*?(\\d{4})"); 
  SetInfo(Info, sHtml, "Quality" , "Качество:.*?>(.*?)</span>\\s*?</div>"); 
  SetInfo(Info, sHtml, "Country" , "Страна:.*?>(.*?)</div>"); 
  SetInfo(Info, sHtml, "Genre"   , "Жанр:.*?>(.*?)</div>", true); 
  SetInfo(Info, sHtml, "Duration", "Длительность:.*?(\\d\\d:\\d\\d:\\d\\d)"); 
  SetInfo(Info, sHtml, "Actors"  , "В ролях:.*?>(.*?)</div>\\s*?</div>\\s*?</div>", true); 
  SetInfo(Info, sHtml, "Director", "Режиссер:.*?>(.*?)</div>"); 
  SetInfo(Info, sHtml, "Poster"  , 'class="preview".*?<img[^>]+src="(.*?)"');
  SetInfo(Info, sHtml, "Kinopoisk", "<span[^>]+imdb_logo.*?>(.*?)</span>"); 
  SetInfo(Info, sHtml, "IMDb"     , "<span[^>]+kp_logo.*?>(.*?)</span>");
  SetInfo(Info, sHtml, "Description", "(<div[^>]+description.*?</div>)");
  Info.Values["Poster"] = HmsExpandLink(Info.Values["Poster"], gsUrlBase);
  return Info;
}
void SetInfo(TStrings Info, string sHtml, string sName, string sPattern, bool bCommas=false) {
  string sVal;
  if (HmsRegExMatch(sPattern, sHtml, sVal, 1, PCRE_SINGLELINE)) {
    if (bCommas) sVal = ReplaceStr(sVal, "</a>", "</a>,");
    sVal = Trim(HmsRemoveLineBreaks(HmsHtmlToText(sVal)));
    if (LeftCopy (sVal, 1)==",") sVal=Copy(sVal, 2, 9999);
    if (RightCopy(sVal, 1)==",") sVal=Copy(sVal, 1, Length(sVal)-1);
    if (bCommas) sVal = ReplaceStr(ReplaceStr(sVal, ",", ", "), "  ", " "); 
    Info.Values[sName] = sVal;
  }
}

///////////////////////////////////////////////////////////////////////////////
string StripHtmlComments(string sHtml) {
  TRegExpr RE = TRegExpr.Create('<!--.*?-->', PCRE_SINGLELINE);
  try {
    if (RE.Search(sHtml)) do sHtml = ReplaceStr(sHtml, RE.Match(0), ''); while (RE.SearchAgain);
  } finally { RE.Free; }
  return sHtml;
}

// ----------------- Формирование видео с картинкой с информацией о фильме ----
bool VideoPreview() {
  char sHtml, sVal, sVl2, sFileImage, sImage, sTitle, sInfo, sDescr, sCateg, sTime, sLink, sData;
  int xMargin=7, yMargin=10, nSeconds=10; float nH, nW; int nSecDel, nCnt, n, i;
  if (HmsRegExMatch('--xmargin=(\\d+)', mpPodcastParameters, sVal)) xMargin=StrToInt(sVal);
  if (HmsRegExMatch('--ymargin=(\\d+)', mpPodcastParameters, sVal)) yMargin=StrToInt(sVal);
  if (Pos('--lq', mpPodcastParameters)>0) {
    nH = cfgTranscodingScreenHeight / 2;
    nW = cfgTranscodingScreenWidth  / 2;
  } else if (HmsRegExMatch2('--pr=(\\d+)x(\\d+)', mpPodcastParameters, sVal, sVl2)) {
    nH = StrToInt(sVl2);
    nW = StrToInt(sVal);
  } else {
    nH = cfgTranscodingScreenHeight;
    nW = cfgTranscodingScreenWidth;
  }
  Variant Folder = PodcastItem.ItemOrigin.ItemParent;
  
  sHtml  = HmsDownloadUrl(Folder[mpiFilePath], 'Referer: '+gsUrlBase, true);
  sHtml  = HmsRemoveLineBreaks(HmsUtf8Decode(sHtml));  
  TStrings FilmInfo = GetInfoAboutFilm(sHtml);
  sTitle = FilmInfo.Values["Title"];
  sCateg = FilmInfo.Values["Genre"];
  sInfo  = "<c:#FFC3BD>Страна: </c>"+FilmInfo.Values["Country"]+"  <c:#FFC3BD>Год: </c>"+FilmInfo.Values["Year"];
  sInfo += "\r\n" + "<c:#FFC3BD>Режиссер: </c>"+FilmInfo.Values["Director"];
  sInfo += "\r\n" + "<c:#FFC3BD>В ролях: </c>" +FilmInfo.Values["Actors"];
  sInfo += "\r\n" + "<c:#FFC3BD>Качество: </c>"+FilmInfo.Values["Quality"]+"  <c:#FFC3BD>Длительность: </c>"+FilmInfo.Values["Duration"];
  sInfo += "\r\n" + "<c:#FFC3BD>Рейтинг Кинопоиск: </c>"+FilmInfo.Values["Kinopoisk"]+"  <c:#FFC3BD>Рейтинг IMDB: </c>"+FilmInfo.Values["IMDb"];
  sDescr = FilmInfo.Values["Description"];
  sImage = FilmInfo.Values["Poster"];
  
  sFileImage = ExtractShortPathName(gsCacheDir)+'\\videopreview_';
  if (Trim(sTitle)=='') sTitle = HmsHttpEncode(Folder[mpiTitle]);
  sDescr = Copy(sDescr, 1, 1500);
  
  TStrings INFO = TStringList.Create();
  INFO.Values['prfx'  ] = gsPreviewPrefix;
  INFO.Values['wpic'  ] = IntToStr(Round(nW/4)); // Ширина постера (1/4 ширины картинки)
  INFO.Values['xpic'  ] = '10';
  INFO.Values['ypic'  ] = '10';
  INFO.Values['urlpic'] = sImage;
  INFO.Values['title' ] = sTitle;
  INFO.Values['info'  ] = sInfo;
  INFO.Values['categ' ] = sCateg;
  INFO.Values['descr' ] = sDescr;
  INFO.Values['mlinfo'] = '8';             // Максимальное количество строк блока info
  INFO.Values['w' ] = IntToStr(Round(nW)); // Ширина картинки
  INFO.Values['h' ] = IntToStr(Round(nH));
  INFO.Values['xm'] = IntToStr(xMargin);   // Отступ текста
  INFO.Values['ym'] = IntToStr(yMargin);
  INFO.Values['fztitle'] = IntToStr(Round(nH/14)); // Размер шрифта относителен высоте кортинки
  INFO.Values['fzinfo' ] = IntToStr(Round(nH/20));
  INFO.Values['fzcateg'] = IntToStr(Round(nH/22));
  INFO.Values['fzdescr'] = IntToStr(Round(nH/18));
  sData = '';
  for (n=0; n<INFO.Count; n++) sData += '&'+Trim(INFO.Names[n])+'='+HmsHttpEncode(INFO.Values[INFO.Names[n]]);
  INFO.Free();
  sLink = HmsSendRequestEx('wonky.lostcut.net', '/videopreview.php', 'POST', 
  'application/x-www-form-urlencoded', '', sData, 80, 0, '', true);
  if (LeftCopy(sLink, 4)!='http') {HmsLogMessage(2, 'Ошибка получения файла информации videopreview.'); return;}
  
  sLink  = HmsSendRequestEx('wonky.lostcut.net', 'videopreview.php', 'POST', 'application/x-www-form-urlencoded', '', sData, 80, 0, '', true);
  
  // Делим количество секунд видео информации на количество картинок (через какой период они будут сменяться)
  HmsDownloadURLToFile(sLink, sFileImage);
  for (n=0; n<nSeconds; n++) {
    nCnt++; if (nCnt>nSeconds) break;
    CopyFile(sFileImage, sFileImage+Format('%.3d.jpg', [nCnt]), false);
  }
  
  char sFileMP3 = ExtractShortPathName(HmsTempDirectory)+'\\silent.mp3';
  try {
    if (!FileExists(sFileMP3)) HmsDownloadURLToFile('http://wonky.lostcut.net/mp3/silent.mp3', sFileMP3);
    sFileMP3 = '-i "'+sFileMP3+'"';
  } except { sFileMP3=''; }
  MediaResourceLink = Format('%s -f image2 -r 1 -i "%s" -c:v libx264 -r 30 -pix_fmt yuv420p ', [sFileMP3, sFileImage+'%03d.jpg']);
}

// ------------------------------- Создание ссылок на видео файл или серии ----
void CreateLinks() {
  char sHtml, sData, sTitle, sLink, sHeaders = "", sVal, sQual, sSeason, sEpisod; 
  int i, n, j; THmsScriptMediaItem Item; TStrings FilmInfo; TRegExpr re;
  if (!HmsRegExMatch("^http:.*", mpFilePath, '')) return;
  
  sHeaders = "Referer: "+mpFilePath+"\n\r";
  
  if (LOG==1) HmsLogMessage(1, 'Загрузка страницы');
  sHtml = HmsDownloadURL(mpFilePath, sHeaders, true);  // Загружаем страницу сериала
  sHtml = HmsUtf8Decode(HmsRemoveLineBreaks(sHtml));
  
  // Получаем со страницы информацию о фильме
  FilmInfo = GetInfoAboutFilm(sHtml);
  sTime  = FilmInfo.Values["Duration"];
  sTitle = FilmInfo.Values["Title"   ];
  sTitle = ReplaceStr(sTitle, "/", "-");
  if ((Trim(sTime)!="") && (Length(sTime)<9)) sTime += '.000';
  
  TStrings LinksList = TStringList.Create(); bool bGroup=false; char sGrp="", sGrpQual="";
  // Ищем ссылки на видео файлы
  sHtml = StripHtmlComments(sHtml);
  
  if (HmsRegExMatch('accordion_head.*?accordion_head', sHtml, '')) {
    // Если переводов больше чем два
    RE = TRegExpr.Create('accordion_head(.*?)<div class="clear"></div>\\s*?</div>\\s*?</div>', PCRE_SINGLELINE);
    try {
      if (RE.Search(sHtml)) do {
        HmsRegExMatch('(<a.*?</a>)', RE.Match, sTitle);
        sTitle = HmsHtmlToText(sTitle);
        HmsRegExMatch('.*\\((.*?)\\)', sTitle, sTitle);
        Item   = PodcastItem.AddFolder(sTitle);
        CreareLinksGroup(Item, RE.Match, FilmInfo.Values["Poster"]);
      } while (re.SearchAgain());
    } finally { RE.Free; }
    
    
  } else if (HmsRegExMatch('film_title_link', sHtml, '')) {
    CreareLinksGroup(PodcastItem, sHtml, FilmInfo.Values["Poster"]);
  } else if (HmsRegExMatch('(<div[^>]+no_files.*?</div>)', sHtml, sVal)) {
    CreateErrorItem(HmsHtmlToText(sVal));
  } else {
    CreateErrorItem('Невозможно найти ссылки на видео.');
  }
  
  // Если есть, для кучи создаём ссылку на трейлер
  if (HmsRegExMatch('/film/index/trailer[^>]+data-rel="(.*?)"', sHtml, sLink, 1, PCRE_SINGLELINE)) {
    sLink  = HmsExpandLink(sLink, gsUrlBase);
    CreateMediaItem(PodcastItem, 'Трейлер', sLink, FilmInfo.Values["Poster"]);
  }
  
  // Если установлен параметр - показываем дополнительные ифнормационные элементы
  if (Pos("--addinfoitems", mpPodcastParameters)>0) AddInfoItems(sHtml, FilmInfo);
}

///////////////////////////////////////////////////////////////////////////////
void CreareLinksGroup(THmsScriptMediaItem Folder, string sData, string sImg) {
  string sName, sLink, sVal1, sVal2; TRegExpr RE;
  
  RE = TRegExpr.Create('film_title_link(.*?)</div>', PCRE_SINGLELINE);
  try {
    if (RE.Search(sData)) do {
      sLink = ""; sName = "";
      HmsRegExMatch('<a[^>]+data-href="(.*?)"', RE.Match, sLink);
      HmsRegExMatch('(<a.*?</a>)'             , RE.Match, sName);
      if (Trim(sLink)=="") continue;
      sLink = HmsExpandLink(sLink, gsUrlBase);
      sName = HmsHtmlToText(sName);
      if (HmsRegExMatch2('^(\\d+)(.*)', sName, sVal1, sVal2))
        sName = Format('%.2d %s', [StrToInt(sVal1), sVal2]);
      CreateMediaItem(Folder, sName, sLink, sImg);
      
    } while (RE.SearchAgain());
  } finally { RE.Free; }
}

///////////////////////////////////////////////////////////////////////////////
void GetLink_TreeTV() {
  string sID, sQual, sData, sLink, sHtml, sHeaders, sPost, sRet, sHeight, sAvalQual, sVal; 
  int g, p, n, nMinPriority=99, nPriority, nHeight, nSelectedHeight=0; TRegExpr RE; TStringList LIST;
  
  sHeaders = mpFilePath+'\r\n'+
             'X-Requested-With: XMLHttpRequest\r\n'+
             'User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.102 Safari/537.36\r\n'+
             'Accept-Encoding: identity\r\n'+
             'Cookie: test=1;\r\n'+
             'Origin: http://player.tree.tv\r\n'+
             'Cookie: mycook=4b8e1d40782a71bb90c23a7a164f047c\r\n';
  if (!HmsRegExMatch('/player/(\\d+)', mpFilePath, sID)) return;
  // Загружаем страницу с фильмом, где устанавливаются кукисы UserEnter и key (они важны!)
  sHtml = HmsDownloadURL('http://tree.tv/player/'+sId+'/1', 'Referer: '+sHeaders, true);
  // Подтверждение своего Fingerprint (значения mycook в куках)
  sPost = 'result=4b8e1d40782a71bb90c23a7a164f047c&components%5B0%5D%5Bkey%5D=user_agent&components%5B0%5D%5Bvalue%5D=Mozilla%2F5.0+(Windows+NT+6.1%3B+WOW64)+AppleWebKit%2F537.36+(KHTML%2C+like+Gecko)+Chrome%2F56.0.2924.87+Safari%2F537.36';
  sData = HmsSendRequestEx('tree.tv', '/film/index/imprint', 'POST', 'application/x-www-form-urlencoded', sHeaders, sPost, 80, 0x10, '', true);
  // Получения и вычисления значений g, p, n через запросы к /guard
  sData = HmsSendRequestEx('player.tree.tv', '/guard', 'POST', 'application/x-www-form-urlencoded', sHeaders, "key=1", 80, 0x10, '', true);
  if (HmsRegExMatch('"g":(\\d+)', sData, sVal)) g = StrToInt(sVal);
  if (HmsRegExMatch('"p":(\\d+)', sData, sVal)) p = StrToInt(sVal);
  sData = HmsSendRequestEx('player.tree.tv', '/guard', 'POST', 'application/x-www-form-urlencoded', sHeaders, "key="+Str(g % p), 80, 0x10, '', true);
  if (HmsRegExMatch('"s_key":(\\d+)', sData, sVal)) n = StrToInt(sVal);
  if (HmsRegExMatch('"p":(\\d+)'    , sData, sVal)) p = StrToInt(sVal);
  sData = HmsSendRequestEx('player.tree.tv', '/guard/guard/', 'POST', 'application/x-www-form-urlencoded', sHeaders, 'file='+sID+'&source=1&skc='+Str(n % p), 80, 0x10, sRet, true);
  
  HmsRegExMatch('({[^}]+"point"\\s*?:\\s*?"'+sID+'".*?})', sData, sData, 1, PCRE_SINGLELINE);
  if (!HmsRegExMatch('"src"\\s*?:\\s*?"(.*?)"', sData, sLink)) {
    HmsLogMessage(2, "Не удалось получить ссылку на медиа-поток Tree-TV.");
    return;
  }
  HmsDownloadURL('http://api.tree.tv/getreklama?_='+VarToStr(DateTimeToTimeStamp1970(Now, true)), sHeaders, true);
  
  HmsRegExMatch('--quality=(\\w+)', mpPodcastParameters, sQual);
  
  bool bLogQual = HmsRegExMatch('--logqual', mpPodcastParameters, '');
  
  HmsRegExMatch('id=(\\w+)', sLink, sID);
  sData = HmsSendRequestEx('player.tree.tv', '/guard/playlist/?id='+sID, 'POST', 'application/x-www-form-urlencoded', sHeaders, '', 80, 0x10, '', true);

  sAvalQual = "Доступное качество: ";
  // Еслии указано качество - выбираем из плейлиста соответствующий плейлист
  if (sQual!='') {
    LIST = TStringList.Create();
    RE   = TRegExpr.Create('RESOLUTION=\\d+x(\\d+).*?(http.*?)(\r|\n|$)', PCRE_SINGLELINE);
    try {
      if (RE.Search(sData)) do {
        nHeight = StrToInt(RE.Match(1));
        if (RE.Match(1)==sQual) { MediaResourceLink=' '+RE.Match(2); nSelectedHeight=nHeight; }
        LIST.Values[FormatFloat("0000", nHeight)] = RE.Match(2); 
        sAvalQual += RE.Match(1)+',';
      } while (RE.SearchAgain());
      if (nSelectedHeight==0) {
        LIST.Sort();
        if      (sQual=='low'   ) n = 0;
        else if (sQual=='medium') n = Round((LIST.Count+1.01) / 2)-1; 
        else                      n = LIST.Count-1;
        sHeight = LIST.Names[n];
        MediaResourceLink = ' '+LIST.Values[sHeight];
        nSelectedHeight = StrToInt(sHeight);
      }
    } finally { RE.Free; LIST.Free; }
    sAvalQual = LeftCopy(sAvalQual, Length(sAvalQual)-1);
    if (bLogQual) HmsLogMessage(1, mpTitle+": "+sAvalQual+" Выбрано качество - "+Str(nSelectedHeight)+" (по установленному ключу --quality="+sQual+")");
    
  } else if (mpPodcastMediaFormats!='') {
    RE = TRegExpr.Create('RESOLUTION=\\d+x(\\d+).*?(http.*?)(\r|\n|$)', PCRE_SINGLELINE);
    try {
      if (RE.Search(sData)) do {
        nHeight   = StrToInt(RE.Match(1));
        sLink     = RE.Match(2);
        nPriority = HmsMediaFormatPriority(nHeight, mpPodcastMediaFormats);
        if ((nPriority>=0) && (nPriority<nMinPriority)) { nMinPriority=nPriority; MediaResourceLink=' '+sLink; nSelectedHeight=nHeight; }
        sAvalQual += RE.Match(1)+',';
      } while (RE.SearchAgain());
    } finally { RE.Free(); }
    sAvalQual = LeftCopy(sAvalQual, Length(sAvalQual)-1);
    if (bLogQual) HmsLogMessage(1, mpTitle+": "+sAvalQual+" Выбрано качество - "+Str(nSelectedHeight)+" (по приоритету форматов видео)");
    
  } else {
    if (HmsRegExMatch('(http.*?)(\r|\n|$)', sData, sLink)) MediaResourceLink = ' '+sLink;
    
  }
  
  if (Trim(MediaResourceLink)=='') { HmsLogMessage(2, "Не удалось получить ссылку на m3u8."); return; }
  
  // Получение длительности видео, если она не установлена
  if ((Trim(PodcastItem[mpiTimeLength])=='')||(RightCopy(PodcastItem[mpiTimeLength], 3)=='000')) {
    float f = 0;
    //sHtml = HmsDownloadUrl(Trim(MediaResourceLink), 'Referer: '+mpFilePath+'\r\nOrigin: http://player.tree.tv\r\n', true);
    sHtml = HmsDownloadUrl(Trim(MediaResourceLink), 'Origin: http://player.tree.tv', true);
    RE = TRegExpr.Create('#EXTINF:([\\d\\.]+)', PCRE_SINGLELINE);
    if (RE.Search(sHtml)) do f += StrToFloatDef(RE.Match(1), 0); while (RE.SearchAgain());
    PodcastItem[mpiTimeLength] = HmsTimeFormat(Round(f))+'.001';
  }
  
  if (Trim(MediaResourceLink)!='') {
    sLink = Trim(MediaResourceLink);
    MediaResourceLink = '';
    if (HmsRegExMatch('--hwaccel=(\\w+)', mpPodcastParameters, sVal)) MediaResourceLink += '-hwaccel '+sVal+' ';
    MediaResourceLink += '-headers "Origin: http://player.tree.tv" -i "'+sLink+'"';
  }
}

///////////////////////////////////////////////////////////////////////////////
//                      Г Л А В Н А Я   П Р О Ц Е Д У Р А                    //
// ----------------------------------------------------------------------------
{
  if (LOG==1) HmsLogMessage(1, '+++ Начало получения ссылки');
  
  if (PodcastItem.IsFolder) {
    // Если это папка - создаём ссылки внутри этой папки
    CreateLinks();
  
  } else {
    // Если это ссылка - проверяем, не информационная ли это ссылка
    if (LeftCopy(mpFilePath, 4)=='Info'     ) VideoPreview();
    else if (Pos('/player/', mpFilePath) > 0) GetLink_TreeTV();
    else MediaResourceLink = mpFilePath;
  
  }
  
  if (LOG==1) HmsLogMessage(1, '--- Конец получения ссылки!');
}