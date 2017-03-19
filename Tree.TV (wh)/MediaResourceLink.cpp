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
             'Cookie: mycook=dd2638574fe86c5f7283eefe716b34e9\r\n';
  if (!HmsRegExMatch('/player/(\\d+)', mpFilePath, sID)) return;
  // Загружаем страницу с фильмом, где устанавливаются кукисы UserEnter и key (они важны!)
  sHtml = HmsDownloadURL('http://tree.tv/player/'+sId+'/1', 'Referer: '+sHeaders, true);
  // Подтверждение своего Fingerprint (значения mycook в куках)
  sPost = 'result=dd2638574fe86c5f7283eefe716b34e9&components%5B0%5D%5Bkey%5D=user_agent&components%5B0%5D%5Bvalue%5D=Mozilla%2F5.0+(Windows+NT+6.1%3B+WOW64)+AppleWebKit%2F537.36+(KHTML%2C+like+Gecko)+Chrome%2F56.0.2924.87+Safari%2F537.36&components%5B1%5D%5Bkey%5D=language&components%5B1%5D%5Bvalue%5D=ru&components%5B2%5D%5Bkey%5D=color_depth&components%5B2%5D%5Bvalue%5D=24&components%5B3%5D%5Bkey%5D=pixel_ratio&components%5B3%5D%5Bvalue%5D=1&components%5B4%5D%5Bkey%5D=hardware_concurrency&components%5B4%5D%5Bvalue%5D=4&components%5B5%5D%5Bkey%5D=resolution&components%5B5%5D%5Bvalue%5D%5B%5D=1920&components%5B5%5D%5Bvalue%5D%5B%5D=1080&components%5B6%5D%5Bkey%5D=available_resolution&components%5B6%5D%5Bvalue%5D%5B%5D=1920&components%5B6%5D%5Bvalue%5D%5B%5D=1040&components%5B7%5D%5Bkey%5D=timezone_offset&components%5B7%5D%5Bvalue%5D=-360&components%5B8%5D%5Bkey%5D=session_storage&components%5B8%5D%5Bvalue%5D=1&components%5B9%5D%5Bkey%5D=local_storage&components%5B9%5D%5Bvalue%5D=1&components%5B10%5D%5Bkey%5D=indexed_db&components%5B10%5D%5Bvalue%5D=1&components%5B11%5D%5Bkey%5D=open_database&components%5B11%5D%5Bvalue%5D=1&components%5B12%5D%5Bkey%5D=cpu_class&components%5B12%5D%5Bvalue%5D=unknown&components%5B13%5D%5Bkey%5D=navigator_platform&components%5B13%5D%5Bvalue%5D=Win32&components%5B14%5D%5Bkey%5D=do_not_track&components%5B14%5D%5Bvalue%5D=1&components%5B15%5D%5Bkey%5D=regular_plugins&components%5B15%5D%5Bvalue%5D%5B%5D=Widevine+Content+Decryption+Module%3A%3AEnables+Widevine+licenses+for+playback+of+HTML+audio%2Fvideo+content.+(version%3A+1.4.8.962)%3A%3Aapplication%2Fx-ppapi-widevine-cdm~&components%5B15%5D%5Bvalue%5D%5B%5D=Chrome+PDF+Viewer%3A%3A%3A%3Aapplication%2Fpdf~&components%5B15%5D%5Bvalue%5D%5B%5D=Native+Client%3A%3A%3A%3Aapplication%2Fx-nacl~%2Capplication%2Fx-pnacl~&components%5B15%5D%5Bvalue%5D%5B%5D=Chrome+PDF+Viewer%3A%3APortable+Document+Format%3A%3Aapplication%2Fx-google-chrome-pdf~pdf&components%5B16%5D%5Bkey%5D=canvas&components%5B16%5D%5Bvalue%5D=canvas+winding%3Ayes~canvas+fp%3Adata%3Aimage%2Fpng%3Bbase64%2CiVBORw0KGgoAAAANSUhEUgAAB9AAAADICAYAAACwGnoBAAAgAElEQVR4Xuzdd5xc1Xn%2F8We2r7qEekGLCqYEIToyAYSdgGPAgH8GOwlYoklgOnFJXIJwSWLj0DGIuhgcY5MEcIcYI4yx6CpgEKorVFFBvWz%2FfZ%2B7e1d3R7O7M7MzuzPS5%2Fh1vLsz95577vuO%2BOc7zzkxy%2FHWaI0jNMUj1A9VH6s%2BWn2Y%2BsDm3xPdwXK9uEF9jbr%2FvkT9PfW3YxZbFT1B409sHsd%2FVjT36CGJXqvSAd6jLXxtrl9T1%2FGfe1pjZu%2FDYq3vIxECryGAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIJC8QS%2F7QrjlSgbYH5Z9UP1l9kvqoDF95m8bbpV6kPiDDY7cM52n9r%2FrZR88faHWvnWTlm06w3naqXqzI2BVXaKTZ6i%2BpP69A3S%2BZVGucZo1JHbiPHRS733Lu876PEXM7CCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCOS1QE4EigrNT5Tieepnq3uAnpftFc36KfVfqreZZlfozcnNPbOBul%2FSL%2F2UwnSfSpuNAD0vP15MGgEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEsizQbQG6QnNfgn2q%2BqXqh2T5PrM2vK8TX6n%2BkPqCdK5SoZPOVb9O3X%2FPTPOp%2BJQqFab7FFs1AvROIs%2BYUWBrh37SGgou1kgHqn9gscZx1hhbbNb4Q7t%2F2hzTC528CqcjgAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggEAXC3R5gK7g3Pczv0p9ejL3uk6rrX%2FNXrWf2GJthD5AJx5mv7WV9mOVcZcHq7Cn3xbbVvuWvW6X2Mf0v352k71hx9lg%2B5Ku0VF7Wwfcoz6z3QO369031Aerdzym%2BS7sU9XPUa%2BIDFxbb%2FbqIrOFq83%2B8RSz0pTu26d4j4L0t23azAO1iPnN9zT%2BdOqXbFZHt7hPvP%2ByjbVb7Ax7XN8n6H1%2Fdec%2F71feO1jB%2Ba3CeUb9aZs5vbYFavrMHvrdvxDiofpNem%2FnPoHITSCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCwnwjsHShe9uAAK2j4Z93%2FFBXRHqCff9LP6xQaTlZ%2Fwh687MN0bBScT9B5X1G%2FMNnz66zBrrE%2F26dspLYPH24XK%2FT9K4Xo37Fjkx2izeN2WJ1u8AUloFXqZ9h%2FKaD%2FuS2xW7Xt%2BrW6Slttvt64Rf3xDmeg0DvYnlzBd7CVe9tjJhzKq9K9T1F%2FR9ud3%2Fus6vRHmk37m1QD9Kbha%2Br%2By77%2BRH%2FbvvP0Oxp%2FVnit%2FaHDO9jXDuj0HujX3tnHdpd%2Bxwoa77b7rvAHm7hNu%2F84%2FZu53Eqrv2x3Xrt1X3PkfhBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBDYVwVaB%2BhX%2FmiM1Rc9o%2FDvHWtU4ff90z%2BwIDQs01LVjTcqOPyMgsN5qWAoOB%2Bk429S96rzlNoa26mN0Z9TlffJdqR5lp%2FZtt1q7f%2FZ%2F9mNNsH%2B2obaBfZ7RekjEwbo63Xpm9W96jz5VqdDf6%2Bu4DvVAD28SIV%2BuV79oHfMlq9MP0D38ao1n1t%2Bufx7K%2B4e%2FfUcC9BftYPsbRthl%2Bn7GtlqnQvQG2M27YFrtUz7q%2FbA5e3uMR%2FMf9r9n9GxvXXsT7J1P4yLAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAKZFdgToE%2BtLLOSmkoN39fKdn%2B%2BdeVsEB5erMr0N1MJ0BWeX63xvqfeJ51pz7ON9nl73n5mn8xKgO5V6B6ae8V5ewH63Zr8N9RTLyXOQIAewvVSgH6yAvRHVIE%2BJKUl3PfQe4B%2B%2F%2B%2Bt7J1bVEX%2FB%2FOHkwttq5VpWYJL7W%2FsPT2L7FXGdypA9%2BXvTfm%2B2X9qf%2FNH9bO%2FFdada%2Fd%2BaVOL4bSZw%2FT7r7RM%2Ftv6Aorvav%2FPVld8qz18iX%2F%2FgoYAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAjkusCdAnz7T10X%2FPwV%2F16vy3APCvdtFj%2FW08l0%2FVzX6p%2FXmAiusP1MV63e1%2FN1QONmK6oYWbSh84YQ%2FDq5%2FcN0pA79vc4Pwu0EDf0v7gf%2BdjbIfKa6%2Bx%2F5i%2F673PmOj7WEt0N7HSlpd7057Rwnkn1te%2B7S2lf6xnWZP2TIt5L5aVemnWE%2Ftgb7Ctmtd%2BFftf%2FW6j%2FFtLe9%2BuR1iXr3ue5qXWqGuslFReYNq2c%2B0aqvXnucvKqZdZf9qx6jeea1u%2BIiWAP0E7Ve%2B3LZpifZFdrwqogs1txcV8JpGMfMF3GvUB6pPVh%2BgHu5z7hXyjeq%2B57m%2Fr6A7uKdoBbpXV7%2Br3k%2F9bPXyOOOP9Pcf1T1v9bG8%2BbXPVF%2BjrgC970lmE39rNnyz2TC9d4PeG9Q3CMbtnQ%2FMLj7N7P1V2nZdr23cZvaKVho%2FeLiWgj9VQ2k%2BwXG%2BhXej7u0cbbu%2B3H5pD2jG2%2B3f9HRu07y%2Fq%2B29a%2FTudC1B31NiYdshzQtsmv2muZr%2Bd3an3Ba3vOZ%2Fj7N1Ou9CPaOD9dxX2l32RPAzPM%2Bv84w%2BAd%2BzTwfjnC6PAs3ld3Z4cJlD9DxmKaMeEvd1hXXWW2n0Z%2FVlimN11mq7yF7R7ubr7W59JnycO%2FTOpfZyy3X872ma%2F%2BN2QjCXmfYT61Vwx0RriPkD8QfU1GL6QDQUTOmwUnzaTH8I8pv2Gzv%2FyWIbsKnRhq2ptxkzGlrGmjGjwNYMK9TS7QX6Akq1TZ%2F5ed1alf49vRr3oPkTAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQRyUGBPgH7FfZ9TuPikNRZ8SmGiNtxuo3mlemn1I3p3rs2c%2Fv3gqOkzv6nlqo%2BympJ%2FrK%2BcMuXhggU%2FOrPhwIJ%2FUsjp%2B4p%2FQVHnLXaiotNaO0tR6WTtZ36Twmsll4pRf2f%2Foaj6TAXk8S2%2BAv23isvPtWcV8Y7UuH%2BjkLc%2BqFD3wPyzWgL8jwqZL9K%2B5l9WZPsn%2Fe77m0%2BzQ4N9zcsUCHvFue%2Bjfpqu76%2F%2FTuNdprMqNaOwAn2wjrzTTrJ7Ndd%2FVnTeqAi5afl1D7Y%2FqV6s%2Fn%2FqQ9SPUX9RfYl6hfqJ6l4droA7OGeMehig%2B%2Ftz1A9TT7QcvQfzPu5Y9YPVFwfEph3gmwr4VYHuAXoQzKuSvFyV2l%2FQmA%2F7eP629kkvKDB7QcfNX66QXe9ddIoOVb77Y82xn%2B7jfM3vQZ33zpM64Sn1euXH19oXZfgVBen3yOFWe1JX6a9gerJC7mdaBeh%2BmZ36UoAH5EfbB3ZDcG%2BmryL0s%2Fe1BP4R%2Bu0aPe1v2a%2BluyYI43%2BuwPvXitFLNed%2Fss%2FpaxPD9dr99t%2ByW6Hr3KSCbQ%2FQPWA%2FQ%2B8mqkD3CvWL9LWHybZQVfMvSHGUPi%2FXKBx%2F2I7TMz7HvmTn25vBuVv0pYTPaYZn68sO4xXmn2tXahYLgmsGAbo%2FgPumP2HX3tVbWxM8ru0Kluq1r%2BizXNsE2Ua7%2FIGp%2BsLInFRWYLDLHzgjGK29f0%2FtXpQ3EUAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEECgKwX2BOjTZk7RhSs7DNB9dsGxsQtsV%2FkFNnZJta0efquCyLNeeuwzr39sZ78LfqFQ81KF2uES6eG%2B4uHfZyksv1I5Zvz78TeeaAl3r0x%2FVhGvB%2BgvKaT9D4XMzyh67atg12u2v60g9Q%2BqUH5C71%2BmcDu6p7kH7LfYPPupgvBeCsK3KIL%2FjEb7uh3Vagn3dxR%2BPxBMZrX6bHUvuI9Wi4f7dP%2B1Xo9fpj369yF630Nmr0T3fPbj6r3beL679Lp%2Fb%2BFkdQ%2FY%2Fe%2Fn1P0a%2Fnc0QPeQvkpdgfyBf6cQXaF%2B3dtmpypMj%2BmR3qcg%2Fgh9IeETHuKr%2FUXh%2BpPatvtqhfE%2FfVlD3aIXfan0UvVpfoD2gl%2Bo%2BPsfVZX%2F62Ap9WKF6221p%2BT1X%2FrSw2MKsMt0X75%2FeYWq%2FN9ROH6G1g2oVyQeNq84%2F72idK9CD5dqX6Qq%2F%2FM092%2FbL%2FR1gwZ9Dpoq29sK0L06%2FT%2F0RQKvXO8rl%2FB4D8zDCvjw3Pix7rRPSPXwpgD9%2Fuqmz%2Fv0mf4tiFv0pY9J%2BtyeqfB8Q5s3G74RDdCvuG%2Byqtav0blP6MskzwbbHVz24AAF7P4Ni%2FM17v%2BqUv0JAvQOVTkAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAgZwSiC7hrnJlZY2xxotUZfvf7c7yivvGK0B8UnuiT7G6oh3HfXDAJ%2BrfavzBtRsO73uwIs46RdknqyK5rQA9PlCPhtzR63YUoD%2BkyuL%2F0dLtv2gO0P3cX6sy%2BsuqfP%2BNliO%2FWkt6R8eOhu%2B%2B%2FHv8HuhnK%2Br9QNXtS5qXKNca6BrRw%2FLT1T1s9kD9L%2BoKpPUFgaZwO5kA3QNvz2g9QN%2B70r7pnn0l8NfUPXz2ynZfxt2XsPdr91KPD9B9aXWvWPeQXEvCT9ZW3I8cpGXdm%2FY5t8NH7gnQV%2Bo%2B%2Fkv3cckn2gzQvZJ%2BtMxW6csCAyVzn%2Bq7P6Mq7ljLUvJ7nsx6fQngfAXeP1QduS%2Bn%2Fqy%2BDOHH3qXa%2FjCsji79Hn2mL6ku%2FNOqHr9Urrfo6XlQ31GAHg3BfdxOB%2BhX3HeOQu7b9Pn9f3bvlb4sQMctXMK9144%2F2I6eM8TyR%2BFoPX2V3DfqGwSxYMmAxzXua%2Fo3dKLt7PEDK995jl5fqYDely%2BgIYAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIBAjgvsCdCvvbOPVZc%2BpQCwxsp2fz6oqo226TM%2FafUFm%2B3By980X8a9pOYn2kf7xS%2B8Nbbo3946%2FsafNCwaMd8%2BUr3vcC2bXaH4vEfWA%2FTfKu72fdKf18LwhwT7ipsi3aX2iKqpH9GC375cezRAf1SvP6zFxsPAPQzQv6QAuKfm%2FWkF6LsUoDeF0t48KPd9zz0nfUnd3xun7pmrB93JBuh%2BXo%2Fm83xVbw%2FEEzUP7L3ifZ26732ufcu1D3tTiw%2FQ%2FbUF6p7b%2BvgVytFlcLUC9MFxAfoi7Z%2F%2B67e0P7oCdF%2FOPUEFelNFumlx9B5aEv2vZXSa4u37tAC7loNP0Hw%2F8galw3%2Bv0H%2B3YvBJcvfw%2FEv2DxrpVoXxfi%2BuNl47xe%2FQO6uDpeG%2Folr3L9jrdpWO%2B5oW0b9az8iXhW%2BvAv1eOTyhxdp%2FoUXmk61A9yp6X2J%2Brwr0K%2B89yuoL%2F1uf8%2Bu0tPqv2ngQe788baZ%2F8%2BEyG77m21pxQdsdFDxnD17mm9a3bv7vaFf5ZxXO%2F1zB%2BresrvhWe%2FgS%2FzYEDQEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEclxgT4DuE%2FVwsaHAQ0VV1zb%2Bi%2FaKXq69oksUCH5Oy1OX6e%2BHtUa4r5RudsV9lw5aXH79935%2F%2FF9dpvj6TVVMn6MI1fciv01xapEC5o9ULRy%2BdoN2yPYl0z%2BnqukzFNP%2Bk03Qkt6t%2F249GVP9%2BFoFq78Plms%2FSZG8t2gV%2BS5Vf5%2BpENb3LX9M0b23L2qP7IsU2p6uUNmvdbai3GubA%2FH3bbNqrH8b7H9%2BnV572T5UkPsnLYa%2BQzPxvcXfU%2FdKcw%2FMfcl1Lxz2PcZ3qi9U92pwn6Xve%2B4B98fUfUl2X6Zd1d%2B6x6bzwj3S%2FW%2F%2F3bPXw9W9wnyL%2Bmnqvqx7tPke6FpeXUFx4oDdg%2FwqdQ%2FgfY7etqv%2FRn2Yus%2FZQ30F6OM1H1%2BZ%2FUtNJvbIC7r8KA2t%2FdV%2FpGXiF9%2BjF31ePocr1H0P93nqJzSPF7OJGuAO7U9%2Bii1qPc3mv96Q6zRJn63zbtT9e7C9Rv9%2FliLxYxS6%2F6e%2BylCoiP2HMrsy2EvetDz8ZxSgP6dd3tdrB%2FajtKv5F7VnQKUW1H9fe9lfHiwdf5hGGaP3ff%2FysC3Qsz%2Fdrg8Ccd8D%2FSN93cED96%2FrWU7Wub4%2F%2BiDbFuzf%2Frq%2BSHC5XqnSsvd3KXbfpuf0v7qWL%2F9%2BgN06XGPq8x2bZcNXf81mzKizaff7txRUtT%2FNH2o7rTFm0x74uoL353XQSgXkN%2Bvnk1rC%2FQV92aTarr2zVP9OjtI%2Fj4t1zHf082j9HKKQvmk3ABoCCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCOS8QHxm7cF4hYI%2FX6L6XM2%2Bb7A0dWPs2wocf6LA0dcZD9odo9%2B%2B%2Bs7l79z1hOLPYxVfehh%2BnsLRL9rBNkU9rO7%2BjarEvX1XwfCfFVi39fcdWt48DLr9eA%2FKvbo8bFcoWr1XldHxy7Av0ZWnq855liqcRypY%2FY6u4xXwX1Bdul%2Brn0Li72m%2Fbq8y9%2Fa8Fim%2FVIHuZs33Zs38KQW1LwYBtwfgHmK%2Fqu5V3V4lfqJ6hfoOdQ%2BcN6t7wO3Hedjuobsqu5vvsem9DyN%2F99fvWlo9aL58u5%2Fv5%2FmS7n%2Br7pXjYfOl4GepL4285oG4h%2FBeve6V6d58rn5dH8ObV8P7dSqa%2F25eUr6HwvHPq0C6pxYS%2BNREZfaqqn9IVebvND0PU6RsirL3VNv7%2Fut%2B3zpWLr6a%2F9P6csI5beyFHi6jfoq%2BWPC1YO%2F2pub7oE9R7f9bmufRcvEQ2%2Fc%2F98D7N%2FrSwh32M%2Fm%2F3PK3n%2BOvlSr4v0Gv%2BrLwHrhHl45v1F9PKpb36vZeqnf3oPwBbRJwvZ6m733%2BvL7AcaFGLdWXF3ysn%2BiLAMcFXzaI2Vf1lQ5vn9bMfmNHfF%2BrJnytZbLhL40Fn7KiuipVpvu3EV6wmpKrrXLq7r2O8%2Bry3aXfsYLGu622eK3OuVHHTNO%2FleG6lK%2Fvf7%2B2NLhVXzY5RAH65QrXv7zXSg57DcoLCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCQKwJ7B%2BhJzKzRGr0Ue09qmsQ5mTrEA%2FQ%2FqTL9MVVxl6rGuTPtOZ3s9dy50TyU93B7jLoH5968btuXUPfl6ZuWqN%2B7eSDv4b9XxHuL25N9hl66Ke6s6ck%2Fdn%2FI%2FrBzqUX3QPcAPdkWuz9YPqCd5lXm9%2F%2B9NRT%2BLuHy7H7mlfcO1ioNt%2Bo3%2FwbC09rf3JccaGrTZ%2Fo3HS5V92853KT3fOkCGgIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAII5IlA8klq8w0pPPey7LAcuktv0%2BPkb2uxeK80v1RVx51pr%2BjkSZ0ZIOPnvqsRfdlyX0a9vHn0bfpZpe7V84m%2BLLBBr2t%2F86BKPWwexIfLxje%2FPlkvPaJe0XxMCgG6n%2BEP2x96rjRfwv0c1aNfpYp930892dZugH7ZgwNUUX6GVluYo%2BDbN5dvu82YUWBrh35SQfrFOsjD8g9UcT5O56qEv%2FGHWg5eywI0b3WQ7OQ4DgEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEul0gpQBd4flozdj3ivafXdYWat9w37t8rfYi973Lv6nFwXu0LGGe%2BjS8pts3vvafudN2aSoeBvsS7h6CezGzB%2BcT1MPl2sPZ%2Bn7lb6gfqn585P2w%2Bjxcpt2XjdfS7d68gF1boQcrtKcYoHfLQ2%2FjwYTV574cvLffaVH%2FZKvQO65Az51PAzNBAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAIGuF0g1QP%2Bjpnhy108zs1c8RcO9lNkh82O0MES%2FN6XHHtybP3R%2F%2BPncCNDz%2BekxdwQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQSyL5B0kqrq8%2Fs1ncuzP6XsXmGahn8gu5fI%2FdEn67EfnPo0%2FeH7hyBfGwF6vj455o0AAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIBA1wgkFaArPJ%2Bu6dzXNVPK3lVmaugrsjd8Ho2sxz5Z000jRPcPgX8Y8rERoOfjU2POCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCHSdQIcBusJz34h7vnph100r81d6V0P6buL1mR86f0e8XlO%2FLeXp%2B0brR1os5qQ0BBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAYJ8RSCZAf1Z3e3q%2B3%2FEZuoHn8v0msjH%2F6zTo7SkP%2FJwCdCelIYAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAvuMQLsBuqrPr9ad3pXvd3u3buCafL%2BJbM7%2FEQ0%2BNeULXKMQ3WlpCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAwD4h0GaArvB8kO5wsXqffL7T9Zr8OPWt%2BXwTXTH31EN0Jx2nEN2JaQgggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggEDeC7QXoHt18VX5fodeQn9Pvt9EV81%2Fji40MaWL3aMA3YlpCCCQRYGL5lnP4hobUlBovQoarKghZsF%2FuxsbrKGowHYVFNlHQybYhhkxa8jiNBgaAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEENjnBRIG6Ko%2Bn6A7n5fvdz9fN3Bkvt9EV86%2Fny72gnpqIfqRCtGdmoYAAhkW8OC8tN4OVGheHovZzup621xfatt61FmjX6q60EqKY9a%2FvsH6xOqsoK7Y1j16pK1WvB68T0MAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEhNoK0A%2FTENc2FqQ%2BXe0RdpSo%2Fn3rRye0YeonslekXS03xcAbpT0xBISWBGoxXMUBE1YW9itqv%2BYkN377RhxUW2fWOJLX%2FycKtpD%2FiSOebbbgwvaLSaHr1t6Z3jrTqlB8LBCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACTcsAR5uqz4%2FQ360qirfbRttu65Xe7FLa1bRCcMwKrMTKrZcym152QM5Rvq0ZeRl96u1DneK9tvnUEv30vl29l%2FrHUh8yo2dUabSNcXNJ9FonLjpZ53olevJtgkJ0J0%2FYpr5mQ4sKbUSs0HYX1NvCmce04CZ%2FhU4c2Z3Xv2KuVdTV2wGNxbb9oQn2fiduY985VZH5lHk2vLTOBqiqeok%2BDztz6eamv2k9agtsfKzACmI7bfmDk%2Byjrp7f5fNtpP5TO7AoZqvvPdLWJXt9zb1Yx45tKLTC8p62mBA9WTmOQwABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQSaBBIF6Pfp9en%2BZo1yrQ22TGnn7na9SoMYfYwVBtlNbrQrNI2ZKU%2FFg%2Bnl6tHVj8v0d6n6FvX9JEB3t5vUZyQNOFMBupMnbN0ZYPuEuvP6BOh7fyTCgNrfKW6wRQTorY3kM1Dh%2FYi6Elvx4OGph%2FeTZ1nR2J421kcddawtYl%2F0pP87xoEIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAQOsAXdXnA2Wy3l2qg5rzpVav%2BNyrzXvZQOutmLzYPFBuCte32Fr9%2F2b91ahXe9tgGxcc291tgybgaxmn3tbqlFXqXnHu%2BVOP1IfI%2BhlVukKWK9DDe%2FAq9MlJ39AghehOv1frzgDbJ9Od1ydA3%2FvzkOsBetKf%2BCwcOHWWlRX2t3HFjbbpvonBf4zSaufPtvK%2BpTa%2BqMA2dmactC7OSQgggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIBAHgu0qkBXgP5l3cstvkz7Wq027SG5B%2BYejBcFVdh7ty22RkH6muCNvjYs6N3dfqgJfCWtSVTprPhwOq2BsnhSojlmad6%2BH%2Foydf%2FZcfuKAnSn36t1Z4Dtk%2BnO6xOg7%2F15IEBv%2Bx%2FTlNk2oqTc%2BvlWBx%2BVWax3jY1rrLcyre2x5v5jm%2F9Dm%2BB0N61r0DIgJVZcWGtr%2Fdhpb9iw%2BkY7YGsPW9jR%2Fukd%2F%2FPmCAQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEBg%2FxCID9Df020fsk1b7m6ylUE1%2BSBVYnt1eXttnS3S7uhbld300G7oFapcXxJUrg%2B0g%2FTK3unrR%2FaBKtx32FDtJx5fsb5DKxZv1DLqvhz8EDs4WELeq%2BF9n3VfKH6TrQiCfYX9wTEe2HtlfL3VqRZ%2BpUbdZJ%2FQFwCWBJXw%2FdVHqhd18DR9i2Nfur1pf%2Fc9zccYrb5Vva1gvU7veaGoV%2BL77978en7fIxJcu6p5LH%2Bvpvl3v65%2FQWGMelj17gsB%2BF7s1c1j%2Bvuj1DclmEs4pi8x78esUN%2Bh7kvR%2B7L6A9SHJpiLD%2B3bT%2FsXIHyP93D%2B%2FrHwKvwhqkBXLX90P%2FQaXatOFoW6VtFwbRW%2FUmy7rKhxd9WU9076bGHMtmpf6w%2Bi%2B5y3F2Ar5DtQFxrYUGB1pVtt6T2Tg4l02C6ZY4NidTZES10H3%2BxobLBqXXtFTYOV%2B37r0T3H469fVGalNTv0rRC%2FS%2B0Tfc%2Fhe19T8%2BrbWGQHadyGPtW2aEeJxaL7YuvDWNxQZ0MLGqyoQB%2FGOu3vXlRvqxRc%2Blr%2FLS0aoBfV2oqGIhtZV2u9NNeY9oSvL9QHZ4E%2BuLMmt%2BAH5176sj7WJTa8IWY9%2FVi%2FhsLQGs3pw4ePaloloqN26ev2Mfn0Kim1dT%2F6q%2BBD0arNaLSCNW%2FZwfVmPWW2NlqpfO0i67Njs%2F5x6R%2B1nAt07Ubfwz7%2BHi%2F7iw1o3GmjfY71Bbb2oaNtddz9j9Ae8EP93M3FtqhPvR0U0%2F1Hj9E9NSS713iwPHkfG1FQZ%2F0btc%2B3n6vAOPjM1RXbGB9b11tVeby%2BA6TW0RcYLp0vI52j%2B%2FdKbX24tX9Fgj3Qw3E6Mo%2BO09Gx8e%2F7vY3pq%2Bex27Y8OslW%2BWdc3qP0GfDPym4P1aP%2FrqLnh5%2Fx4LWY7Rz5C3v%2FL%2BdbUZ%2BddnCs3tY%2FeGLwHxMaAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIBABwItAbrysRN17Gw%2FPgzEy62Pqs%2FHd4joe6R71boH6N42Kofarv%2F3YHtAkI%2FuaR6sf2gLg4DdK9t9%2F%2FRo82XjdyokDiXN0AgAACAASURBVM%2F1SngP0D3E9%2Bv4edHmAXx%2FheTblCnWKsafozfPa3VEH%2F3V0T2kG6B7SL1U3YPwRM1DaA%2FFe0berAqEml7z8Drcb71cvx%2Bs7uF7eEz8mGGw7aF6dD%2F28HjPkj2Mb23UNIqP75mxzylsft8fqCtCTdj8elpR4Cb1Gc0HhAF6TPNUuhz90sHhG%2F%2F74knr%2F%2BNtBcu7lu6yhWEo3FaAfulbNrywwYamGp5PfcUqior1jYq45mGqvvKwo7HOercXoA872urD4Li2xtZ6WBk%2F1pfesVE11TZYS2Bvve8oW9QqVK23bQrO%2B3iwGT3PQ%2Bb4SuGWALfIqhVkFuhx%2BzcaWjcFnou32qLQKwylFX4n3A%2BhMWYbFFT7Nz7abZe9YkMKihQ2K3xdsGXP8whPuuov1ivRFwm8clmfoGEeisdfwIN8vbZBXxTwD07Qwnv051heYkvCLyRcusB6F2y1sUEA3MOW%2B37eYWAdHTfZAP38v1hJv1ob7xXZexHW224fR7Pr0Z0BerLPJtGD8y9t6PUD66ut6qGTbFvzFxwKZx5tdTe%2BYmW3TtJ%2F4NpobuNvHX6Y1emzXRbuK68xx%2Br5F%2FhnuKPPC%2B8jgAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAgh4rWJzUy72ff361TDg9rC6vyqo%2BwSVy6k13xfdK8eLlOl4FblXioctrDD3wN3H9muErU5BtIfrDSrG9er1cuVJYYDux%2Fgy8gNUYe2v71ZV%2BAYFzeEe7Z71eTX6dxXa3xIEwlXqXjmuGt9gP%2FP2q%2Bib5uDnJKo0T%2FS6h8fvq3sI7tfwpevDnde9QNgLYL2iOxqMR6%2Fhv3uI7gG756Qeevuxvo24Z5OeU4YV9O7nleeq9m4JxxMF6D6mjzW4eT7%2Bt1eXr1P3%2BXrmXOEvqnnov1Ddw3ifh1fa%2B%2FX9ul5N79fyYxTK91OwP0f5nJ8aBug%2BREzzKlYVepHGVWrdY%2Bfr9%2F%2FD%2Byfd7yFrYbGtvPfI4MIJl1D3kNYnmWp4ftlsVTz3sNFeFa1AelvtTvug8lSrVljcX1MdGQbU7QXoXsUbhsteOb40LlwOK4FV%2FVumLwOs8urdMED3inO%2FJ91jTU2RraicaJs9iN613Q5UWF2u4%2Btqy2zxY0cGSwC0hMv%2Bu8L4OoXia%2B6dYOunv6Wi%2FUY7UOFyPw%2FeS5qq9jdEq8L1qdpRvc2qKifbbl2%2FODzezYobbFEYkjY%2F0L1%2BTK2ysqKP7GAFzoX1xbbM5xo9SMF3UB3u19GXChbOiFnDlfOtv744UOHhvX95oMduW%2BnBrQe0%2FXfZUH1KB%2FqnMfp8o8G2vlexbcQEW%2BzXWTlHS0wo0I6vyk53CfeLX7Hxum4fXxVAZmsqjrN1CosLZT5Sz7F%2FGPhnI0Bvyzj6jDWv7Ut22JL41QTaOzf63mVva0WFehvYXqV5smOFx%2FmXVwqKbUD8ZzzVcTgeAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEENhfBKIB%2Bru66UN9efQPVazoAfcBClV7Bst%2Fp9b8XA%2FCa1QwGT%2BGV5h7RbkXshYr8fSAPVzGfbvC440Kj0tVyR6%2BHgboBQqp45eT36zC4S0KqsPw3AP0wzRVX4e%2BKdj2oksP04NVwpO4iSodk2yA7sd5EbAT%2BvjxBdH%2BfhiEezgdvh9eo61g30N5X8U8UeX8Nr2%2BpPmeEgXozRXje%2B1D76tqe6Dv2a9X40eD%2BuhrUSJf8dkLs32eOudcrS7wlH5tCdAV1JcepLcjS%2FQ31i64fH7J57xCOFqJG1%2BBriDYH0YQnisMX%2FbQIVpAIMl2xRwbr72ePUTdPupYW%2BShb3hqWPEcLO1dbNsfmhB8wyFhgK8lykt3b9Fy2Y1WFB8uxy%2Ff7gFyqwA9ZrXxy817iBwsl60l5RWurw8rtCMV6A211fbBj48PPmBB81C8oTAIuMvCZdZvnG3lm8uCcQoURgdV2%2BHx4Zz1d3E0wG6P7pLXbUyBFmloaLBNDx8XLJcQtPBLAvpCQHlpma1S1XjTkufNvvqCwkcPnKBvwcS1RKG7HxJdyr243FbX7rISfTlgULh0e3QP7nQC9LBa3l0S3Xt0ifWuDNDDL4Ko%2Br3al6jvzF7j%2Fu%2BksMz6hp%2FbkH7qLCsr7W31bS3f3vL5%2BG3TdgZ3%2Fl3Lvg%2F%2BGRuorQcGN3%2FhItHSFEn%2By%2BMwBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQGD%2FEAgCdIXZh%2BqHB%2BjB%2FuKdDdB9nK0axQNuD%2BB9X3RvYYW5L%2FVeryt5lXt0GXdfOn63stR%2BQeW79t9WCwP0Uo001A5p9VTCwN1Xuval5pfqGA%2FQm5pnRV5hvVvdq9yTqaSv0nHJBugennu1uC9b%2FzH1RKtth2G4fwlBYXPQwmv4KtS%2BZHt0RW9fodnn7Jmwh%2B6JvrzgXwrwyvpEAXqiMf2abuDjekW87wnvFeodtXBZe%2F%2BIeOiuKnXfC%2F3jmr%2FvgV6ga5Vq%2Fl6FHmlHrP36mSes%2FvcPo1XH0QC9ZpdtLuphg1VpW59qeB5WVNerCjys2I6%2Fi5aAvYMA3c8Lw%2BX4Cun45dv92GiA3tY%2B1%2BF5wR7UR9n7Hu6HwW5be1jH78EdDdW9yl3V1esW77aN6VY1f%2FE1O6CwwEarCrlma5EtDAPeqXOtn%2FYOP8j3Yfc93v1LAqFvEN7X2bL4%2Fdzd4aJ51rOsumlPhN2ltiistPe%2Fo18WaKht2rc9XLo9%2BpzSCdC9Olv%2FyRjpJmV9beGd4%2FeExPHPp6sC9HRXUWjrX158gO6rEax63cY16D9%2Belbaqt1q9WWIjfVb7SNflUD%2F4Y5NezP4ps1ABfi9wy0FGmpswyPHNe15T4De0X%2FneB8BBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQaC0QBuhX6%2BW7%2FC3fR3ytwtbOVKD7OGFY7tXh4TLuHnh%2FpFzHl2Gv1grX%2Fne4jLuH6V617tXofrwv%2F%2B4tDNA9iPdl3aMtXA7el4j3c%2B7TOde0HJDtAD0Mx6PLosd%2FvKr0Qnwgn%2Bi18Dxfpt3f9zDeM8qmPeVbN8%2FFfGX0RAF69LXoWdHl5v2LBHuWzW86yt%2F3kN0r3%2F2nrz7uP%2F11n0tzmF%2BhX9%2FX%2FDxAL9S1Sv2LA63bgG2%2F%2Bs5nF539TKIA3feo9mDVl9r24LatkDbBTQcvXfmSlhcvswofp60lzMMK6Y4q0H28METWx6Y2DJejy7dHK53D0FfLuhe2Vf3tYWWNlmX3kHfrMoXVF1hNS6gcCfSj9xcfoPt7LcFsdA9yVb3X1djWAY32YXt7Ycfb%2Bf0c0tcOrvYq9%2BZl4puv4cvHDwr3eI%2F6trX3enTsRPuWx%2B9R3tYXDdIJ0C99S8v2a%2BWC6HONzidaUd8VAXpYce9zSPQlgbY%2Bw%2B29Hr%2BEe7NTRb%2FdtuzWE2331HnWt7hG337RN5B8WX7%2FgoKH6vqP9eb%2BdbbBPxc%2BL%2F3HdUhhoy32inUP5YtL7IBMLgufzr1xDgIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCQLwJhgP4zTfgCn3R0D%2FR%2BNjzYVzzd9pGWMN%2BmkNyXce%2Bl%2F1%2Bv5cc9OPew239u1BLoTZXlH2upWO%2BlmHxAsCR6UwsDdD8%2FrGQP3wsD9HCv9X9QkP7zljPzMUAPq77bC9B9pW1fWj1RgN5emJ8o8Peg3JeZ9%2BXu22qRAN0P%2BWaV2b%2B0HaCX7Xrttxe%2Bd8K3EgXofrov264Avc6XLY9WaifzGQv3P%2Fc9sNsK0MNq92QC9EThcnT59roBtrCyIvgmQUsFelCdvVNLq0%2Fas7R6OPdE80snQPfxLpljg2Q0VFZN3ySJNN%2Bz%2FKMyW5rscuHxFfVhlXtdnZVGQ%2FXo%2FvIdPY9EAbqfEwbd%2Frs%2BA2vvmxh8WFu1dAL0jhyDa8%2B3j6lMu1e2A%2FSrZlmv6j42xkPskhJbde%2BRwTdaOt38s6dBDqyvtqqHTrJtvpz%2FjhIbXdrHlsVX3Ld1MX%2BGhSU2%2BP1ttthXLdCYYwsKreC%2Bo4L9LGgIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIdCIQBuqeoo8JjPejeqQXYy1Uf7kujd9TCZd99KXUPv8uDHMiCvc7X2WIlpX20LPvwoMK8RO%2F6mHV61%2F9WFhos477FViup3B5UmffQ0WFLJUA%2FSAF6sG5x0PIxQE%2BmAn2N7s33NE%2B3Aj1czt6rzX1L7HBbZN8L3ZeA9yXZezcbrmz%2BGVlOvl%2BV2Z8VoI9JXIFeWP2XNRf%2F5a%2FOThSge3heU2PLSvtaY8FWG%2BthdH2BrX3o6OCGOmyZrkD3C06ZbSNUoTs0rMROtHy7HxeGvkFoGqnkjk46rEAv0n7YYcVvR8Fvogr06Jgeom4qsoFFxdZHFful4TLdWgJ%2F24jjbHF0D%2Fi2AMP9wz309uXaNxRaj0TLuifzBYX2HtKV87VCQLVVhBXsvkx%2FQx9bEr%2FHfToBekcV6M3P6JB67TWQzQA9DM%2B1d7z%2Fg1mjZe79H2RGWlhFX7%2Fbtjw6yVb53x%2FrbeN21dv6Hx8fLGXRYfMVGGpqrdT3u%2FcVAfrstIP1HNY%2FeKL%2Bc0tDAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBDoUECrIjd6ohompcEJ25XVeHW4B%2BKDbKxi1TBQTTye73W%2BRbXiTUupj9f%2FlwcH%2BjLwTSF5veLegbZJx%2FVXkO7LtnsL9zzvrbO8Hj2sJPdl3MOWbIBep6r20a32E892gN7RHui%2B%2FLnvO%2B5V3on2QE%2B03Hp0r3Kvwh%2BYAHyJXtusnihAd3ffV92zvWjzvdW9ANX3QA%2FHrdLvnsl5aD5OvTTuHC%2Bq9Y%2BFf8ciuh%2B7zvu6zvvXxAG6Vb9vp6668dOHbvvNe6o%2B9otYdA%2F0MFhWZWywhLiGr60utCXRvbQT3HTwUqb3QPcxW%2FY21z%2BE0q22dFdfO9Cr4%2BND8rg90BNWVofhu1eIa%2FnsBT5%2BZwP0qEWwJ%2FZ8G91YZwP8ywha2nthMsu5%2B3lr3rKDGxuth%2FZUX9VQaD21l3b%2Fhgbb5EFreA0P6zeX6bh2viTQ1rMJqtrrdW6hlcWKgur8Ys2zd6JVBtIJ0K%2BcZ4Pra22kxq1NtAf6tYusdPcWO1ifqZJkA3R3WTlHy1%2FIJfqFj3B%2B8asNRJeob2t5%2BrZ8kn09CMAbre%2FSLbawuYJ8WF2h9d9WYos7WnHAw%2F3dPa1Cqxas8P3rfSuA%2BkY7oH6HLQ72TKchgAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAgh0KOAB%2Bqd01G%2BjR3rw7cG1V5YXK2D1CvGivQLWpjM8%2BN6oZcD9nETLrG9VhL5FRZoeqvs%2B5x6wawvf4NxtitA3KaT1sX3PdK9S76Mjoi3ZAH2OguOzujRA9%2FDZQ3QPmD2U9uXTo83f98L%2BRnXP%2FQY3v1mln%2FH7okfP86B7q7oH5F79v%2BfLBE17lHuA7kF4ogDdj000F5%2FHenWvLveA3UPz9vZw9%2FDf5%2BHXi1vC3TPxvpr%2F%2B7r%2BkL33QPcA%2FaDNT197xtp%2F%2FmV7AXpLdWyBlSoY3frIicktMX3FHBtf12B9Ep3jlda7arRktaqDk1nCPVT3MRX%2B%2BrdENqhKe4AqvevjQ9pogK4l5Ku39tAe54frQ9vcps5ScNzLxvuS67U1ttYriP2tVAP0sMre97cu6WmL7zk8eAgtLdh7u04hupZuSDZA95Mve8WGFBTZiFjMdtYVBd%2BwKI5fij4M2r2Ku63l9cMqe1Ww13k1exjgT33FKlQlf4AC%2Bl1Ld9nCwwdZWfgs4pdyTydA93B%2Fa6mN92ebaNUCVagPL2ywoV6hHxegj9DfQ31f%2Bvhn2rJcv74w0FGA3vzlhXH%2BpQB9QWDbiAnJVf9Hn10yv%2FvnqLC%2FjStutE2%2B%2FH143YK64B%2FiEt%2FXPNE4%2FtnfvTMIzzd6Vfz58uorL62ssDHRMvrJzIVjEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAIH9UcAD9Bt047fG37wvv75exam%2BJ7pXhPdU7XhvhcBh%2BO3ve9X5riDsbWwO2scHVeTR5sG4V6H7ku1%2Bru93HlaY1%2BrsD5Wb%2BjU8RPe90ePPTzZA%2F6nO%2FXKXBugeMnsI7fuHq%2BY42Ct%2BUPOte1jte5V70B1fFV6l19oL0MNl3H18z3Q9EPfA26vOfYH6MLNNFKD75T0bDefi1%2Fcqch%2FTg3yfX7i%2FvBce%2B%2Bt%2BvK%2Fe37957n4dPye8ToIA3ef%2FDV3%2Fu4kD9IE7Xr%2F1cysuuqu9AN0vFoax%2Fntby6I3T6rlR6t9umO2pW6rrfTq2qlzrV9JnY3yCmQ%2FOJUAPZxHrNDqPaBV2LpZIaR%2FU6GlRQN0f9GD4vJe9oEH3MES6bUKtVW5rorfmsbttiis%2BE01QA%2F3ZdeXBMo1n93l1bbizuP1XZOYNV66QB%2BIbTZaldGlCtC3jzrWFiWzhLvPNwygg3%2Bq%2FtQLbNeC5irn6H1%2B8TU7oLjUDtSXCAr8HmN9bYUvwe7zGqsdHVS5Ptgr1KOBc7h0e0GxNeq8ZV797GN6NbWH1%2FFLuUeXwy8rtpXJ7iEerlrgS9ErYF439Mim5dPXzrNhtTEb7HP2v6MBut%2BPL1evYDnmz1VfIPhg5tFWN%2F0tVWbHbLg%2Fbw%2Fd2wvQR55om1WpPkZ2ff2LBVtKbUlH1eDxn9tU%2FvbPo57xiLoSW%2FHg4faR24%2Fro6Xx67S%2FuwLyxh22Pvh8NVrs0j9br1i5DdHxvbSiwDrfDiEM3RtqLJbKZySVOXIsAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIILCvCniAfrdu7qpEN%2Bgh%2BQZbFlSHt9c8GB%2BkfKmtKvWPVIm9TXF8X8Xn%2FcxXjN%2FTwoC8t0bw%2FdPjW7IB%2BncVoN%2FbpQG6z9SXZ%2Fcgui0fz3LHqPeM3FaVfm8vQPdDPRf07qF3tHk%2B6Eut%2B5LsiQJ0f8%2BD90RFqh7G%2B1LtYUV7%2FB7o0et4Vb0H6v7lCNUjJ6qg76frL1OAvme7%2BqYBVIHeu3rRk%2F%2B47OyvdhSgB0Hf6zZOy373TlTVHXfzLX%2BG1c7x73tVtkLnbR50phKgB9Xw2gPA9xhXAN6YKMxvtax3fRBeNi2jEGkKpet0%2FQ%2FunRB8MyFoqQbofk5QZb4zCMqjyw%2FsuZKWvffl5u%2BZ3Lo6vS2v8HUF0GMVFgdPTGHyegXdvjTBXu3y%2BTYyVmODw%2F3W9zogEiJHl26PHzO6RHq0oj04pzBYKt6%2FGeJzaSxMIkhvHq8pyI5r%2FuUH%2F2eoz1JhNECPVo4nOGe3nvluN2kvQPfzGnvoeTQH9O05%2B5cewm0Kwq0LfLn94gZbpOpx%2F7ZNUs2fgf4pDyyK2erwCwZapr7Pjs3Bt2N6hJ8NH1uf1y0bi221h%2Fpuq%2FfHyrewXCsY3Dle31yiIYAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIJC3gAfovdfRZbZ3hS7Nv1VLrO7Wtca2yGP%2Fbm1eRe2DuS6770u3ttTCI94C8PC778n3Rvdb9AKtQIhmfxnod9%2Fu66vaEy8Pv0Jx8r3avWp%2BmAP3XXR6g%2B117lbev1u2V2%2F67N6%2Fq9nvxLwvE70depdc6CtB9DC%2Fi9XF962IP0j0c90pxz2bjz4%2BO6cd4LupZnZ%2FneZrvpe65mwfj0eZfAPCq9vBYf9%2FDfj%2B2j3q4nLyH6f5FAG%2BRa92mAP36uCEVoJfVrHxp6tK%2F%2BWJHAbqfGV12va7WNlae2LRvekftkjk2SIGmV94Gm7d7AO97P2uZ8v4KUA9IJUD38yPVzdVbi1ovz%2B7vtwrQi2x5ba2Vl9TYIA9sPbjXtbduKrcV8ZXJ6QTofr2L5llPpcsjVEXcMwxLvZJbVdNbt%2FWwlelUQIfV2EWq4I5Wiieybglr9e2YluBYwb0%2BURseOkrf7lBFvJ8Xv3S779sdHS9umfSWveO9ar1WKwa0VMQXWsJ95RPNzYPpkgIb5KsNePjuy9LvqLPV5aU2ykP5aIDu5zdXz4%2FQMuj9g%2Bel%2By%2FUtgH%2BvA6oteH%2Becm1AL3538ZQLcs%2BrLjItm8sseUdPXP%2FN6Hzhvty9T1621LC847%2BK8L7CCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggMDeAh6gv6GXj8l3nGN1A29m7SaqNHIyoXfWJpCbA1doWssSTu1Ni8X8kXR5u3yeHaR9qgckWoa9vclc8rqN0fLk%2FbUM9qaHjwuWFWjVWgXoO235g5P07Q1azghEq9rjA%2FTunKSvJqBvAI0sbLTFqVSgh3P2L1KU1tuBWmq%2BvL7EdpRob3Qtrb992Dar0X%2BViurKraRHofWrK7T%2B%2FkWHumJb9%2BiRtjr8gkN33jvXRgABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQTyUcADdOUwNjofJx%2Bdc4X%2BWJ61m3AiAvSEvI%2Fo1al7vbNcAbo%2Fkoy2aEiqiuG9KpanzrKyWC8br2rwkkTvtzWZcAl3LQFeXF9syyonBssJtGoE6Bl9lBkfLFcDdN8HXqsV9N%2B6TKsaXNDBXhjtqHiQXlxjQwoKrVe4b7sf7kvXa8n43Vp54aMhE2zDjFjzEiEZF2ZABBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQGD%2FEPAAPX6j7by88%2FjFyTNzE%2BFq1L6UuS9zPkD9oMwMva%2BMco5u5OkENxPTwtpZaFfMsfF1DdbH9xuvq7a1I0%2Bw9R4aXrpA%2B6hvsVEKEstVoVtXW2aLHzsy2KQ%2BYZsxo2l%2FcS05XjCmpyp8VX1eqE3thx1tCxOFkAToWXiYGRwyFwN0LYNfumuHjauvtV2JVjXI4O0zFAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAQIYECNDbhKzVOwvVfQ9yb54H%2B%2F7ivs0wrZWAb8sev319lgL0q2ZZr%2Bo%2B2pC9sdWG9y3T8f2t66ttTeXxtra9p3TlS9a%2Fpswqwv3FfR%2FzklKrundCsMn8Xo0APbc%2F87kYoF822wYUltjgj8psaUf7l%2Be2LrNDAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBPYfAQL0Np%2F1Lr3jlecepKs22Q5Q9wCdtpfAbXrl%2BrhXsxSg%2B1V8yfX%2Bu2yUwvLejfXBwwmWstY%2B0zt2FNjq9irPw1meP9vK%2BxfbuAYt967vRtTq55qHj7L1bT1dAvTc%2FtznYoCe22LMDgEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAIJEAAXqgskp9vvoC9cXqH6ivUd%2Bg3tbO6r5t%2FED1Yer%2B%2B1j1Q9QnqI%2FYvz5tE3W7c%2BJuOYsB%2Bv6Fy90igAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggEBXCeynAfp78n1e%2FSX12eorMuztleqT1E9W%2F6T6oRkePweHW6Y5VUTmRYCegw%2BJKSGAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAQHsC%2B1GA%2FoocnlL%2FpboH6F3ZPEA%2FW%2F089RO78sJdd62bdKkZkcsRoHedPVdCAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAIGMCHiAXqWRfA3yvG4Vmv3ei637EuyV6g%2Bp%2B%2FLsudB8mfdL1aeq%2BxLw%2B0hrvYz7covF%2FJHQEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAgbwR8AD9Dc32mLyZcRsTPVavv9ny3tv67R71mTl%2BW9M1v6vUj8jxeSY5vU06rl9w7JsK0P2R0BBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAIG8EfAA3dc0PytvZtzGRH2B9F%2FZfP3%2FLeqP59ntXKj5fkV9Qp7NO266vkL%2BucFrv1KA7o%2BEhgACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCOSNgAfod2u2Xgadx229buBm%2B1FQdZ7H7Tg9hoXaTHzLoPy8ies07duDqd%2BjAP3q%2FLwJZo0AAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAvurgAfoN%2Bjmb81fAM%2F%2Fv2G32Va7MX9vomnm%2FhTO62M243tmj%2BZh%2FrxnH%2FQbFaDflu%2BPg%2FkjgAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggMD%2BJeAB%2Bqd0y7%2FNv9t%2BV1P27P%2B5YOp%2BA5%2FOv5toPWO%2FCX8a3madbjZVGfTyw%2FLrrpr2Qf87Bei%2Fy6%2BJM1sEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEENjfBTxAHyGElfkFMVPT9QrtupZpr9JvI%2FPrJvaerT8Ffxph21yoEF3L0j8zPX%2FurGkf9JEK0P2R0BBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAIG8EYj5TBWib9WP3vkx62ma5gMJp3qgXl2RHzex9yxH6aUP2pj87Zer2P7%2BvLiz%2FlfYtk33xbQOPQ0BBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBDIL4EwQF%2BnaQ%2FK7akv1%2FQuUn%2BpzWl%2BXu%2F8PLdvou3ZXaC3ftbO5OeebDb5MbMto3P6Ds%2BYYOufnR8bnNOTZHIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIBAAoEwQN%2Bo9wbkrtArmtoX1D1Eb7vdrbeuyd2baH9md%2BltX5W%2BvTZX4fnUJ8zmnZizd%2FmDfvbRVzfHDsjZCTIxBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAoA2BMEBvzF2h5zS1M5Ka3ns66rCkjszBg97VnA5NYl6bdczkZxWin57EwV1%2FiN%2FGYdoAveuvzBURQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQACBzgnEtP%2F5RA0xp3PDZOvsX2vgs1Ia3AN0D9LzqvXVbD0YT7YFIfqvFKKfmewZXXKc5%2F8eoKsdMSYJDAAAIABJREFUFbPY3C65KBdBAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEMiTgAfq5GuupDI2XwWGSrzyPXvRr%2BuMHGZxFlw21TFeqSOFqOViJ%2FlVN%2F%2FtNt3CaAvRZKdwNhyKAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAALdLuAB%2BgzN4qZun0mrCfie55PSmlL6Z6Z1ucyddJuGuj7F4TxEnzhbW8Pnxp7omok1z%2BRmBej%2BuaIhgAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACeSPgAXqlZjsld2a8XFM5Vd1%2Fptd8KfEF6Z3afWedo0s%2Fncbl547Wcu4vmm3Rz25sh%2BjakaXzH1WAPrUbp8OlEUAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAgZQFPECfpbM8sc6Rdorm8VKn5vJDnf2VTo3QTSdv0nX7pXHtuSdr1%2FE%2FpnFi5k65RUN9ec9wLypAn5y50RkJAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQyL5AjgXo03THD3T6rjdohEGdHqUbBvCd6H1H%2BnTa7Zeb3XB%2FOmdm5Jz1GmXgnpEI0DOiyiAIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIINCVAjkUoM%2FUfV%2BRsXv3kXzEvGq%2BE%2F2MTsz43PvMnpneiQHSO9WvqCtHGwF6epSchQACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAAC3SiQIwH6uyKYoF6fMYq3m0fM2IBdMVC6%2B6CHc9tcZDZxnraPP6wrZttyjfn67YjWVyRA79InwMUQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQCATAh6gL9NAFZkYLP0xztCpz6V%2FehtnXqTXH8%2F4qFkc0Hein9XJ8Wedbnbas50cJPnTL9Shj%2B19eJX2QD8o%2BVE4EgEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEOh%2BAQ%2FQG7t3Gnfr8tdkZQpeGX1kVkbO0qATNe6cDIx97l1ayv3qDAzU8RCqdw%2FWDohvCtBjHZ%2FNEQgggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggEDuCHRzgL5eEuPUt2ZNxGPke7I2ehYGzsTXGar6aCn3xWZbBmVhgnuGvEq%2F%2BtcfEjUC9KzSMzgCCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCGRBoJsD9OzH29mP6DP8VB7ReBdnYMybFG%2FPaCvezsD4Td96GKdCcydu1bSqwVQF6JUZuQqDIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAl0k0I0BetctsJ69ReIz%2FJR8vfm56pXqmQjRl2mB9YpEC6xnZN7XKDzfK6H38FyjP0IFekaMGQQBBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBLpQwAP0Kl1vdBdes%2FlSF%2Bnn41122TN0pee67GppXuhUnTer%2BdxK%2FexsiD7lQoXxj6U5mXZPe07huZO2amF4rheXK0CvyMaFGRMBBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBDIloAH6LM0uEe3Xdje1rWyVhmd8D7e1ate4F3XhXeZ8qXO0RlPR86q1O%2BdDdGXqdK%2F4oiUp9LOCfXBw4vFnLSlRcJzf%2B1FBeiTM3lRxkIAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQSyLdBNAfoVuq%2BZ2b63vcb3K%2FqVc7bdpJnNiJtdpf7uTIh%2B03SNeV8mb%2FkKheetHl5ceO7XIkDPpDhjIYAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIBAlwh0Q4C%2BQTc2qEtuLtFFpunFB7rt6h1c%2BCm9f26CYyr1Wrohej%2Bdu2y9Wb%2BBmbjrBxSeO2FLSxCe%2B3sE6JnQZgwEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEOhSgW4I0H%2BoG%2FxKl95k%2FMVO0QsvdesM2rj4Jr3ugXeiVqkX0w3RH7nFbOqXO3vHLyk8d7qW1kZ47u8ToHdWm%2FMRQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQKDLBTxAr9RVp3TdlQ%2FVpRZ03eUSXGm5XvNN3%2F1nzrT4%2Fc8TTcyfVDoh%2BsRDzOa815lbbSKLxVrI2gnP%2FTqPag%2F0qZ25IOcigAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACXS3gAfoMXdR33%2B6C9oquMakLrtPxJXJnJs1zvU0%2Fr%2B943lapY9IJ0ZfNNqs4MYkLJDxkksJzJwtaB%2BG5H3KzAnT%2FXNEQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQACBvBHwAN133fbdt7ugfU3X%2BEEXXCe5Szynw85I7tDsH7VMl6hI8jKVOi7VEP2mr5rN%2BH6SF2h12BkKz50qaEmE537YaQrQZ6VzMc5BAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEukvAA%2FSJuvicrpnAYbpMp5YSz%2Fg0f60Rz8r4qCkO6AvoV6Z4jh%2BfSoheoaXzl72b4kVEE4s5UdCSDM%2F90KMUoM9N9WIcjwACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCHSnQMwvrmC0MfuT8ODcA%2FTca91aid5XHlXq%2FdJwqdQ5qYToHqB7kJ5cS6fyPBhZ4XnwuaIhgAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAAC%2BSQQBuhVmvTo7E78bg1%2FTXYv0YnRfYPvL6gv78QYaZ3qu8%2FPSOvMppMq1ZMN0R%2B5y2zq1R1dzAm%2BkOKe59ExlytAr%2BjoIryPAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAII5JpAGKDP0sROze7kPq%2Fhf57dS3RydE%2BOL1J%2FqZPjJH16Z6rPoxep1B%2FJhOhTLlDg%2FrP2pue3fpHC85bvEaSwbHs47osK0CcnbcCBCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAQI4IhAH67ZrPddmd04EafkV2L5Gh0adpnAcyNFa7wzyld8%2FN0IUqNU5HIXrFKO2D%2FkFbF7xfwfn06JtphOd%2B%2Bh0K0K%2FP0F0xDAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIINBlAmGAPlVXfCR7V12loUdmb%2FgsjDxTY16lXp%2BFsYMhb1PPdMxcqTE7CtE3rdR%2B6yOid1WnP65WeO633NLSDM%2F9%2FPMUoD%2BdLTbGRQABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBLIlEAboFbrAsmxdxOy3GvrT2Rs%2BSyO%2Fq3FvUH8u0%2BNP0YCVmR60eTwft70Q%2FYXfmE3%2Bu%2FDifms3KDz3W21pnQjPfYz%2BCtA3Z%2BnuGBYBBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBDImkAQoHtTaFqlH6OzcyUvt74xO0N3wah36xrfUN%2BaiWsdqUFmqffLxGBtjFGp19sK0R%2B51WzqDX4r31Bw7rfWqnUyPJ%2Bn8HxiFu%2BMoRFAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAIGsCUQD9EpdxWujM9%2FGTFlnS388OPMDd92I63Wpm9Xv6cwlXdd3m89meB7Oz59mghD9i%2F%2F0xXU%2F%2FuGjf6Xw3G%2BpVetkeO5jsf95Zz4fnIsAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAt0qEA3Qp2om2dkH%2FX%2BGz7ZxaybZLbrC4916v52%2B%2BHyNkNZtZGPP847uplIHNIfoF%2BrXr6hPOPnja2Mv%2FXlY%2FKkZCM99SPY%2F7%2BiZ8D4CCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCOSsQDRAr9Ass7MP%2Bq%2F6L7QzNx8cKLyt7mXcM3PWJKmJJX0bfTVcpfq5SQ2b8YOm69pXKUQ%2FIhz5uEO3xV5%2Fr0%2F0QhkKz31I9j%2FP%2BBNkQAQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQ6CqBlgDdL5i1fdCf7fGhnb5rSKub2qC%2FKtUfUl%2FQVbeb%2Beu0eRsenF%2Ff3LtiyfbIrR2i3y9Vn6o%2B0F9353A590lDd8dmry0PD89geM7%2B55n%2FeDEiAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAgh0oUB8gO47dF%2BX8etv0mXaC5Ff0RWfUv%2Bl%2BnsZv3r7A3rQvSUz12y5De11%2Ft4MjVmRmXGTGeVQHXS2%2BnnqJyY6oVIveoiuOcWqLHjuGQzPfbibYxbzu6YhgAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACeSkQH6BX6C4yv4x7Y6vLtA%2FlAfrz6i%2Bpz1ZfkWHXURpvkvrJ6p9U9%2BS5Sv1p9Vnqz6R5vXN03mR1X6pdit1xGx3OvFJHKETX04hlODz3Sx%2BkAN0laQgggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggEBeCuyVbCtYnas7OTKjd5NKgB5%2F4VV6wTcc90R6ifpy9TXqvna6%2F56ojdaLvnb5MHX%2Ffay6B%2BW%2BEfiIDu5ss96fpe4K3v1v7%2FOaz3MZr6b3PrG5T27%2Bu52hu%2Fo22pxKpd65uNFr0R%2FpQCKVt1m%2BPRUtjkUAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAgZwUSBSg%2B87dt2V0tp0J0DM6EQYLBFR%2FnuF2g6rPffl%2FGgIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIJC3AokC9ArdTWaXcSdAz60PSOYD9P4K0L1On4YAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAgjkrUDCzcm1jLvvCO67emembdJlfMlzWvcLVGkKB2W0Av0Zhee%2B8zsNAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQyGuBtgL0qbqrzO2R%2FWyPD%2B30XUPyWmpfmfxz5R%2FaGTsz%2BSwuVoBeua%2FwcB8IIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIILD%2FCiQM0J1DVehV%2BjE6IzS%2F6r%2FQztx8cEbGYpDOCfy630I7a1OmnsVyhecVnZsQZyOAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAK5IdBegH69pnhbRqb5P8Nn22fXTMrIWAzSOYEHR79hl1cd27lBWs6m%2BjxDkAyDAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAALdL9BegO67llep9%2B30NL94%2Buv26P8d1%2BlxGKDzAlP%2B9nX78XOZeBZbNJkKVaBv7vykGAEBBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBDofoE2A3SfmpZxn6EfN3V6msff%2BLq9elsmQttOT2W%2FH%2BCEG163127NxLO4WeG5fz5oCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAwD4h0FGAnpkq9ME%2Fe9s%2B%2FMIR%2B4RYvt%2FEkCfetnWf7%2ByzoPo83z8HzB8BBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBPYSaDdA96MzUoVeuHKr1Y3qg38OCBSt2Gr1Izv7LKg%2Bz4FHyRQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQCCzAskE6BW65Fz1zu2FvrhkjY2tHZbZ6TNaSgJLitfYuJrOPgOvPp%2Bo5durUro2ByOAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAI5LtBhgO7zz0gV%2Br8d9pr9y3vH57jHvj29fz%2F0Nfv6u519BlSf79ufEu4OAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAgf1WIKkAvTlEr9LP0WlLHfO1V%2B2NH5yQ9vmc2HmBY7%2F6qr35%2Fc48g%2BWqPK%2Fo%2FEQYAQEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEMg9gVQC9Mma%2Fgtp30LJ26usesKItM%2FnxM4LlM5fZTVHdOYZnKYAfVbnJ8IICCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAQO4JJB2g%2B9S1lPvT%2BnFO2rexsHSxja8Zl%2Fb5nJi%2BwKKSxXZwdWfsn1F4fm76E%2BBMBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAILcFUg3QK3Q7c9X7pnVbf%2F%2BZl%2B2%2FfnlSWudyUucE%2FuHsl%2B2nv0jXfosuPlEBelXnJsHZCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAQO4KpBSg%2B22oCn2GftyU1i2V%2FXGN7Tp1WFrnclLnBMpfXGO7T0nX%2FmaF5%2F7caQgggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggMA%2BK5BygO4SCtFn6cepaam82qPKjt9Vkda5nJSewGvlVXbCznTNX1R4Pjm9C3MWAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAgggkD8C6Qbo%2FXSLVeqpL%2BX%2BiStesudnnpw%2FRPvATE%2B89g179Y5j07gTX7q9QgH65jTO5RQEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAgrwTSCtD9DlWFPlk%2FXkj5bgvWb7GNg%2FuaR%2FC07At49H3Aui3WMCj1LzuYnabwfFb2J8kVEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAge4XSDtA96mnvR%2F6lE%2B8aZUvHNP9t78fzGDqaW%2Fao39Ix5p9z%2FeDjwe3iAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACewQ6FaD7MGnth1761mrbfcxwHkQXCJS9udqqj07Vmn3Pu%2BDRcAkEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEMgtgUwE6BW6pbnqqS0R%2Ft0jZ9s35k%2FKLY59bDbfmzDbvjkvVWP2Pd%2FHPgbcDgIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIJCfQ6QDdL6Mq9In6MUs9%2BRC9VJXRu49NtTI6ubviqCaBsjdUfZ5Spb%2BH55O177l%2FIYKGAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAII7FcCGQnQXUwh%2BmT9eCElvWtO%2FZPd%2Bce%2FTukcDk5OYMrfvm4%2Ffu645A5uOeoowvMUxTgcAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQT2GYGMBeguohB9qn48krROwbottnFYgfVr6J30ORzYscCywu02bnW9NQxOfkUAs4sVnld2PDhHIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAvumQEYDdCdKOUQ%2F5muv2hs%2FOGHf5O2muzr8m%2FPt3e9MSOHqhOcpYHEoAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAgjsmwIZD9CdSSH67fpxXdJkTw19w8798Nikj%2BfAtgWeHvKGnbc2Fcs7VHl%2BPaQIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIDA%2Fi6QlQDdUVMK0Uvmr7IPjxxi%2Faxof38gnbr%2FzVZvQ%2BattZoJI5Ic52aF5zOSPJbDEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAgX1aIGsBuqultJz7Id%2Bea%2B%2FdNHGf1s72zR1681xb8K%2FJGrJse7afB%2BMjgAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggEBeCWQ1QHeJlEL0606abbf%2FeVJeCebKZK%2BfNNvuSNqO8DxXnhvzQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQACBnBHIeoDud6oQ3auiZ6n37fDO%2F9T3LTtp69EdHscBewRe7vOW%2FfWWZMy26KTJWrZ9LnwIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAq0FuiRA90smHaIXVm212YfssOOqh%2FGwkhB4vXSNTVrQ0%2Bor%2BnRwNOF5EpwcggACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAAC%2B69AlwXoTqwQvUI%2FKtVPbZe89x8W2QefHG%2F99t8Hk9Sdb9ZRw15cY7tP6ejLBi%2FqyHNVee5n0BBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEgh0aYAeXl9B%2Bgz9flO7T6TfU0ts2WfHEqK3oeRR%2BEH%2Fu8Q2nze2g0%2F2zQrO3ZuGAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIINCOQLcE6D4fheiT9eNp9bb3Re%2F%2F86W29PNjCNHjnqCH52N%2BttQ2XTCmnWfrS7Z71fks%2FgUggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCHQs0G0Buk9NIbov0u4hettLulOJ3vopJld5zpLtHX%2F2OQIBBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBoJdCtAXo4k%2BYl3a%2FX34mr0X1P9Oc%2F3cuOq%2B5or%2B99%2B%2FG%2BXrrGTnnO2tnz3KvOb2fJ9n37Y8DdIYAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIBAdgRyIkD3W1OIXqEft6ufk%2FBWC6u22otHLraTth6dHYocH%2FXlPm%2FZqfPGWX1FnzZm%2Boxev17heVWO3wnTQwABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBHJSIGcC9FCneW%2F0Sv09OqHYDR%2BfbbfOnpSTmtma1A2T%2Fmy3%2F%2FnjbQy%2FXK9PZa%2FzbOEzLgIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAII7C8CORegh%2FDtLut%2ByLfn2uybjrB%2BVrhPP6jNVmeTbn7HFvzrxAT3yXLt%2B%2FTD5%2BYQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQKCrBXI2QHeI5mXdp%2BrXvfdHL5m%2Fyn52%2Bho798NjuxqtS6739JA37PPPDbOaCSPirhcE5%2BqVLNfeJU%2BCiyCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAwH4ikNMBevgMFKT30%2B8eou8dpB%2F2rfn2q38fYwfV99onntmywu121r8stXe%2FMyHufsLg%2FHYF55v3iXvlJhBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAIEcEsiLAD30ag7Sp%2BpvD9L37JFesG6LXfiP79ujvz8%2Bh2xTn8qUv3nNHv%2FJx6xhcN%2FIyb7HeVhxTnCeuipnIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAkkJ5FWAHr0jhelT9fe56ue0vF765mr714ur7Otvfzypu8%2BVg%2F7tiD%2Fbtx%2BpsOpjhkem9Ix%2Bf1rV5pW5Mk3mgQACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCOzLAnkboIcPJVKVPlWvHRm8XvrWavv7L6%2B221441nzx91xsXkt%2Bw2lv2E9%2FONyqjw6D83l6tdI7y7Tn4kNjTggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAgggsC8L5H2AHn04CtMr9Lcv7z5Z%2FUgrWL%2FFjv%2FuIrvjgYF2%2FC5%2Fr%2Fvba%2BVVdt3lG%2By1b463hkG%2BVLuH5k%2Bre2he1f0TZAYIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIIIDA%2FimwTwXo0UfYXJk%2BWa819bI%2FDrbP%2FnCpzXh2iI2vGdelj3tRyWKbccaH9r9fHmO7T1mna88KO5XmXfokuBgCCCCAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCDQpsA%2BG6DH33GrQH3wy6Pt%2BMdL7aQXD7DzF4%2BysbXDMvoZWVK8xp4ct8JePnWjvXZhta07abnGn%2BWdwDyj0gyGAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIZExgvwnQ2xJTsD7Rznt4sI2bPdFGLRxt%2FdYNtzOWldvg6oE6x%2FvoNs71UHyDrSvdYM8etMs2D15tKw5ebosnzbWnLlmnoHxuxp4SAyGAAAIIIIAAAggggAACCCCAAAIIIIAAAggggAACCCCAAAIIZF1gvw%2FQsy7MBRBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEE8kKAAD0vHhOTRAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBDItgABeraFGR8BBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAIC8ECNDz4jExSQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQACBbAsQoGdbmPERQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBPJCgAA9Lx4Tk0QAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQyLYAAXq2hRkfAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQCAvBAjQ8%2BIxMUkEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAgWwLEKBnW5jxEUAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQTyQoAAPS8eE5NEAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEMi2AAF6toUZHwEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAHdtF5lAAAGZ0lEQVQEEEAgLwQI0PPiMTFJBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAIFsCxCgZ1uY8RFAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEE8kKAAD0vHhOTRAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBDItgABeraFGR8BBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAIC8ECNDz4jExSQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQACBbAsQoGdbmPERQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBPJCgAA9Lx4Tk0QAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQyLYAAXq2hRkfAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQCAvBAjQ8%2BIxMUkEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAgWwLEKBnW5jxEUAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQTyQoAAPS8eE5NEAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEMi2AAF6toUZHwEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAgLwQI0PPiMTFJBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAIFsCxCgZ1uY8RFAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEE8kKAAD0vHhOTRAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBDItgABeraFGR8BBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAIC8ECNDz4jExSQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQACBbAsQoGdbmPERQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQAABBPJCgAA9Lx4Tk0QAAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQyLYAAXq2hRkfAQQQQAABBBBAAAEEEEAAAQQQQAABBBBAAAEEEEAAAQQQQCAvBAjQ8%2BIxMUkEEEAAAQT%2Bf3t2TAIAAIBAsH9rU%2FwgXAGRc5QAAQIECBAgQIAAAQIECBAgQIAAAQIECBAgUAs40Gth%2BQQIECBAgAABAgQIECBAgAABAgQIECBAgAABAgQIECBwIeBAv5hJSQIECBAgQIAAAQIECBAgQIAAAQIECBAgQIAAAQIECBCoBRzotbB8AgQIECBAgAABAgQIECBAgAABAgQIECBAgAABAgQIELgQcKBfzKQkAQIECBAgQIAAAQIECBAgQIAAAQIECBAgQIAAAQIECNQCDvRaWD4BAgQIECBAgAABAgQIECBAgAABAgQIECBAgAABAgQIXAg40C9mUpIAAQIECBAgQIAAAQIECBAgQIAAAQIECBAgQIAAAQIEagEHei0snwABAgQIECBAgAABAgQIECBAgAABAgQIECBAgAABAgQuBBzoFzMpSYAAAQIECBAgQIAAAQIECBAgQIAAAQIECBAgQIAAAQK1gAO9FpZPgAABAgQIECBAgAABAgQIECBAgAABAgQIECBAgAABAhcCDvSLmZQkQIAAAQIECBAgQIAAAQIECBAgQIAAAQIECBAgQIAAgVrAgV4LyydAgAABAgQIECBAgAABAgQIECBAgAABAgQIECBAgACBCwEH%2BsVMShIgQIAAAQIECBAgQIAAAQIECBAgQIAAAQIECBAgQIBALeBAr4XlEyBAgAABAgQIECBAgAABAgQIECBAgAABAgQIECBAgMCFgAP9YiYlCRAgQIAAAQIECBAgQIAAAQIECBAgQIAAAQIECBAgQKAWcKDXwvIJECBAgAABAgQIECBAgAABAgQIECBAgAABAgQIECBA4ELAgX4xk5IECBAgQIAAAQIECBAgQIAAAQIECBAgQIAAAQIECBAgUAs40Gth%2BQQIECBAgAABAgQIECBAgAABAgQIECBAgAABAgQIECBwIeBAv5hJSQIECBAgQIAAAQIECBAgQIAAAQIECBAgQIAAAQIECBCoBQam5gleRFxmLQAAAABJRU5ErkJggg%3D%3D&components%5B17%5D%5Bkey%5D=webgl&components%5B17%5D%5Bvalue%5D=data%3Aimage%2Fpng%3Bbase64%2CiVBORw0KGgoAAAANSUhEUgAAASwAAACWCAYAAABkW7XSAAAM7ElEQVR4Xu2dXYglRxXHu3ZWJKCIiIQQogaVfVjEqCgEEWcUFEVByUMEBVEIKIqCH6APYTooCIqioOQhgoLkIaAoCkpAmQGRCJrsB7PJuBmz2ew6Zo1fkSRq1L1Wdd87986d%2B9F9u7rqnKrfvG531an%2F%2F%2ByPqnOrqk3BHwqgAAooUcAoiZMwUQAFUKAAWCQBCqCAGgUAlhqrCBQFUABgkQMogAJqFABYaqwiUBRAAYBFDqAACqhRAGCpsYpAUQAFABY5gAIooEYBgKXGKgJFARQAWOQACqCAGgUAlhqrCBQFUABgkQMogAJqFABYaqwiUBRAAYBFDqAACqhRAGCpsYpAUQAFABY5gAIooEYBgKXGKgJFARQAWOQACqCAGgUAlhqrCBQFUABgkQPeFRgMinXb6JuNKe7w3jgNZq0AwMra%2Fn4GPwTWlm19w0Jru59eaDVHBQBWjq73POarg2LLJpabZRUWWORYz3rn1DzJlJPbgcY6CSzbZcnSMJDwGXQDsDIwOfQQLbAGU4kFtEKbkGh%2FACtRY2MN67%2B24H6sqJaE039AK5YpCfULsBIyU8JQ%2FjesX81JLIrwEkxSHAPAUmyexNCXAIsivETTFMUEsBSZpSFUuyQc2CXhop8Gt20RfkPDWIhRngIAS54naiNy9Ssb%2FNYSYLnxUc9S63LcwAFWXP2T6v0%2Fw%2FpVA2ABraScDzcYgBVO6%2BR7agkspwdF%2BOSzwu8AAZZfPbNuzQKr2n%2FVcIZVacVO%2BKxTpvXgAVZryXhhlgL%2FsvWrteH%2BqzbAsm1RhCelGisAsBpLxYOLFPj3oNi0oCrbzrCGbVKEJ70aKQCwGsnEQ8sUeHZiw2jLGdaoaaC1TGT%2BnZP05IAfBewM62D%2F1YrAcoEALT92JNsKM6xkrQ03MFe%2Fsol0sP%2BqA7AowoezTWVPAEulbbKC%2FuewfjUCVRdgUYSX5a20aACWNEcUxmOB5WZX7paGqsbQEVgsDRXmQKiQAVYopRPuxwKrql95BBbQSjhfugwNYHVRj3eLp4b7r3oAFtAiv44oALBIik4KuPqV%2B3WvJ2BRhO%2FkTnovA6z0PA06omcmDjx7XhKOxsFO%2BKCOyu4MYMn2R3x0FlgH5wd7AhZLQ%2FFZEC5AgBVO6%2BR6cvWr0f3tfS0JJ0RjU2lyGdR%2BQACrvWa8MVTgaVu%2Fsgl0cH6wxxnWSHOglXn2AazME6DL8C2wqg%2BmjvZeBQCWC5c7tLqYpvxdgKXcwJjh2yXhofODgYBFET6m6ZH7BliRDdDa%2Fd9t%2Fer41PnBQMCiCK81aTzEDbA8iJhjE5GBBbRyTDo7ZoCVqfFdh%2F2PGecHA86wKMJ3NVDp%2BwBLqXGxw7bAOnJ%2BMAKwKMLHToTA%2FQOswIKn0J1bDrr9V9N7ryIBi%2BM7KSRVwzEArIZC8dhYAWnAspGxPyuTBAVYmRjtc5hPzjk%2FGGuGNRwb0PJpstC2AJZQYySHZWdYB%2FWrSUhFBha%2FHEpOGk%2BxASxPQubSjFsO2rEe1K%2BEAYsifOKJCLASN9j38BQAiyK8b9MFtQewBJmhIZS%2FTdWvBM6wnIwc39GQTCvECLBWEC3nV5QAi3pWokkKsBI1to9h%2FXl4f%2Fvk7QxCZ1ij4fPLYR%2BJELFNgBVRfG1dKwQWRXhtSbYkXoCVmKF9DuevM%2B6%2FEj7DquQwhjOzfeZFyLYBVki1lfelFVgU4ZUn3kT4ACsdL3sdiVsO2mSp9l8pqmFNakI9q9cMCdM4wAqjs%2FpeEgAWvxyqz0Luw0rAwjBDeGLO%2FVcaalhTCjHTCpMyvfTCDKsXWdNrNCFgUYRXnJ4AS7F5oUJ%2FfLj%2Fatb9VwpnWE42dsKHSh7P%2FQAsz4Km2FyCwKKepTRRAZZS40KG%2FacF918pnWGN5KOeFTKRPPQFsDyImHoTCQOLmZay5AVYygyLEa4F1mDW152l3OneVRN2wndVMNz7ACuc1ip7cvUr98GJlIFFEV5PagIsPV5FifTKVQurepd7kdCvhLO0LM1acUcUkem0sQIAq7FUeT545dlqdpUDsNw26tI8B2hJznSAJdkdAbE9%2Fkz9wYkMZli12g5a1wAtAak3MwSAJdUZAXFdfqpYP14fyckHWDW0Nszzim0BFhDClAIAi5SYq8D%2BkxZWddE9N2BtmxcUG6SGPAUAljxPxES0%2F5dMgTVaGr6IpaGYZBwGArCkOSIonv0r4w%2BmZlPDOqx%2Faa4FWoJSkqtjJZkhKZbLl%2B1S0Iw%2FmJopsJwlpbkeaEnJTWZYUpwQFsf%2BY8WWDamqX2VXw5r24qotwr%2BMIryEFAVYElwQGMP%2BBYA1aYu5kdWIhDQFWBJcEBjDHx6ef35Q6Z3uXVUuzStZGnYVsev7AKurggm%2Bf3m3%2FuDEvLpVpsCqN5WeAFoxUx5gxVRfaN%2BXzhWbdv9VCbBmGOSgdRJoxUpdgBVLecH9Xjpb778CWHNMGtgi%2FE0U4WOkMMCKobrwPi89sPj8YLZLwgnfzGspwsdIY4AVQ3XBfT7662J97dji84MAqzJw27yB4zuhUxlghVZceH8X77P1K7tZctHeK4B1YGJpbqaeFTKlAVZItRX09dgv6%2FuvAFZjs0rzJqDVWK2ODwKsjgKm9vrF7eXnB5lhHXF9w6xThA%2FxfwFghVBZSR%2BP%2Ftz%2BMtjg%2FCDAOmJoad7KLCtEmgOsECor6ePivcWmDbWqX7EkbGxaad4OrBqr1fFBgNVRwJRev%2FDT6tfBpQeemWFZ190G0ncAqtD5D7BCKy64vws%2FHtevmGHNMcqB6t2AKlYaA6xYygvrd%2B9Hdv%2FV8P52loQzzSnNewFV7LQFWLEdENL%2F3vctsOyB5xGsmGENjXEzqlsAlZA05XiBFCNix%2FHIPfUHUwHWBKhuBVSx83K6f2ZY0hyJFM8jdx%2B%2B%2FyrjGVZp3g%2BoIqXh0m4B1lKJ0n9g77v1%2Fe1Nfv2bBtk8sClMrNI5bT4IrCRnvMK8kiynztj2vm2BZQ88ZwssV6f6EKDSkL0AS4NLPcf4%2B7vq84MZAqs0twGqntPLa%2FMAy6ucOhvbu%2FPo%2FVeJ17BK81FApTFbAZZG1zzGvPvNYv34cDtD8jMst%2FT7GKDymD7BmwJYwSWX1eHu1zMAlgPVJwGVrMxbLRqAtZpuybx1%2Fmv1%2Fe1Nf%2F1r%2BpyQxCrNpwFVMsnqfsVNaTCMpb0C5788%2B%2F4r1TUsN6P6LKBqnw3y3wBY8j3qLcLdL9bbGWadHVQJLAsqJ5b5PLDqLWkiNwywIhsQs%2FsKWBMHnpUX3UtzO6CKmU8h%2BgZYIVQW2sf5zWLLhjbz%2Fis1M6xjdvm3CaiEppj3sACWd0n1NHj%2BdgusiQPPqmZYrk71BUClJ9v8RAqw%2FOiorpXdz9X3t09CSgmwSvMlQKUu4TwFDLA8Camtmd3PqANWab4CqLTlme94AZZvRZW099Cn6vvbxc%2Bw3NLvq4BKSVr1HibA6l1imR089AnhwHKg%2Bgagkpk98aICWPG0j9bzzsfH97cLnGGVThjzLWAVLUEEdwywBJvTV2g7Hxnf3y4KWG5WdSeg6sv3FNoFWCm42HIMD942vr9dBLAcqO4CVC1tzPJxgJWh7Q9%2BWAywSvMdQJVhCq48ZIC1snQ6X9z5QL2dYdm3B3ve6V6a7wEqnRkUN2qAFVf%2F4L3vvM8Ca86B5943jrql392AKrjpCXUIsBIys8lQzt06%2F%2Fxgj8AqzT2Aqok%2FPLNYAYCVWYacuyUosErzA0CVWYr1OlyA1au88hrfec%2FhD6b28Svh2uheqh8CK3kZoDsigKXbv1bRn35XfX%2F7PEj5WBLaD7KWaz8BVK2M4eHGCgCsxlLpf%2FDsO%2Bv723sCVvncnwEq%2FVkiewQAS7Y%2FXqM7%2B7bF5wdXmWG55d819wIqr0bR2FwFAFZGyXH2LUc%2FmLpqDcuB6vm%2FAFQZpY%2BIoQIsETb0H8Tp9fH97Z2WhPZK4hduAar%2BHaOHWQoArEzy4swbq2L7ke8PtphhlS%2F%2BFaDKJF3EDhNgibXGb2Bnbl5%2BfnBWDcst%2Fa69D1D5dYPWVlUAYK2qnLL3Tr9%2B9gdTF8ywSnee8LrfACtlVicdLsBK2t56cKdvKtbtN76XHniegFd5wylAlUFqqBsiwFJnWfuAT7262DSDopoxLbqFwS3%2FXnoGULVXmDdCKQCwQikdsZ9TJ8f1q1nAcqB6%2BTlAFdEium6oAMBqKJTmx06dmHt%2BsDzxO0Cl2dvcYgdYiTv%2B21fU97cfKq7bGdXJhwFV4tYnOTyAlaSt40Hdf2Oxac8Pls5ot%2FR71QVAlbjlSQ8PYCVtb1E88JKqfrX9mkuAKnGrsxgewErc5vuvKzZf90dglbjN2QwPYGVjNQNFAf0KACz9HjICFMhGAYCVjdUMFAX0KwCw9HvICFAgGwUAVjZWM1AU0K8AwNLvISNAgWwUAFjZWM1AUUC%2FAv8HKWr1ps4nyTgAAAAASUVORK5CYII%3D~extensions%3AANGLE_instanced_arrays%3BEXT_blend_minmax%3BEXT_disjoint_timer_query%3BEXT_frag_depth%3BEXT_shader_texture_lod%3BEXT_sRGB%3BEXT_texture_filter_anisotropic%3BWEBKIT_EXT_texture_filter_anisotropic%3BOES_element_index_uint%3BOES_standard_derivatives%3BOES_texture_float%3BOES_texture_float_linear%3BOES_texture_half_float%3BOES_texture_half_float_linear%3BOES_vertex_array_object%3BWEBGL_compressed_texture_etc1%3BWEBGL_compressed_texture_s3tc%3BWEBKIT_WEBGL_compressed_texture_s3tc%3BWEBGL_debug_renderer_info%3BWEBGL_debug_shaders%3BWEBGL_depth_texture%3BWEBKIT_WEBGL_depth_texture%3BWEBGL_draw_buffers%3BWEBGL_lose_context%3BWEBKIT_WEBGL_lose_context~webgl+aliased+line+width+range%3A%5B1%2C+1%5D~webgl+aliased+point+size+range%3A%5B1%2C+1024%5D~webgl+alpha+bits%3A8~webgl+antialiasing%3Ayes~webgl+blue+bits%3A8~webgl+depth+bits%3A24~webgl+green+bits%3A8~webgl+max+anisotropy%3A16~webgl+max+combined+texture+image+units%3A32~webgl+max+cube+map+texture+size%3A16384~webgl+max+fragment+uniform+vectors%3A1024~webgl+max+render+buffer+size%3A16384~webgl+max+texture+image+units%3A16~webgl+max+texture+size%3A16384~webgl+max+varying+vectors%3A30~webgl+max+vertex+attribs%3A16~webgl+max+vertex+texture+image+units%3A16~webgl+max+vertex+uniform+vectors%3A4096~webgl+max+viewport+dims%3A%5B16384%2C+16384%5D~webgl+red+bits%3A8~webgl+renderer%3AWebKit+WebGL~webgl+shading+language+version%3AWebGL+GLSL+ES+1.0+(OpenGL+ES+GLSL+ES+1.0+Chromium)~webgl+stencil+bits%3A0~webgl+vendor%3AWebKit~webgl+version%3AWebGL+1.0+(OpenGL+ES+2.0+Chromium)~webgl+unmasked+vendor%3AGoogle+Inc.~webgl+unmasked+renderer%3AANGLE+(NVIDIA+GeForce+GT+630+Direct3D11+vs_5_0+ps_5_0)~webgl+vertex+shader+high+float+precision%3A23~webgl+vertex+shader+high+float+precision+rangeMin%3A127~webgl+vertex+shader+high+float+precision+rangeMax%3A127~webgl+vertex+shader+medium+float+precision%3A23~webgl+vertex+shader+medium+float+precision+rangeMin%3A127~webgl+vertex+shader+medium+float+precision+rangeMax%3A127~webgl+vertex+shader+low+float+precision%3A23~webgl+vertex+shader+low+float+precision+rangeMin%3A127~webgl+vertex+shader+low+float+precision+rangeMax%3A127~webgl+fragment+shader+high+float+precision%3A23~webgl+fragment+shader+high+float+precision+rangeMin%3A127~webgl+fragment+shader+high+float+precision+rangeMax%3A127~webgl+fragment+shader+medium+float+precision%3A23~webgl+fragment+shader+medium+float+precision+rangeMin%3A127~webgl+fragment+shader+medium+float+precision+rangeMax%3A127~webgl+fragment+shader+low+float+precision%3A23~webgl+fragment+shader+low+float+precision+rangeMin%3A127~webgl+fragment+shader+low+float+precision+rangeMax%3A127~webgl+vertex+shader+high+int+precision%3A0~webgl+vertex+shader+high+int+precision+rangeMin%3A31~webgl+vertex+shader+high+int+precision+rangeMax%3A30~webgl+vertex+shader+medium+int+precision%3A0~webgl+vertex+shader+medium+int+precision+rangeMin%3A31~webgl+vertex+shader+medium+int+precision+rangeMax%3A30~webgl+vertex+shader+low+int+precision%3A0~webgl+vertex+shader+low+int+precision+rangeMin%3A31~webgl+vertex+shader+low+int+precision+rangeMax%3A30~webgl+fragment+shader+high+int+precision%3A0~webgl+fragment+shader+high+int+precision+rangeMin%3A31~webgl+fragment+shader+high+int+precision+rangeMax%3A30~webgl+fragment+shader+medium+int+precision%3A0~webgl+fragment+shader+medium+int+precision+rangeMin%3A31~webgl+fragment+shader+medium+int+precision+rangeMax%3A30~webgl+fragment+shader+low+int+precision%3A0~webgl+fragment+shader+low+int+precision+rangeMin%3A31~webgl+fragment+shader+low+int+precision+rangeMax%3A30&components%5B18%5D%5Bkey%5D=adblock&components%5B18%5D%5Bvalue%5D=false&components%5B19%5D%5Bkey%5D=has_lied_languages&components%5B19%5D%5Bvalue%5D=false&components%5B20%5D%5Bkey%5D=has_lied_resolution&components%5B20%5D%5Bvalue%5D=false&components%5B21%5D%5Bkey%5D=has_lied_os&components%5B21%5D%5Bvalue%5D=false&components%5B22%5D%5Bkey%5D=has_lied_browser&components%5B22%5D%5Bvalue%5D=false&components%5B23%5D%5Bkey%5D=touch_support&components%5B23%5D%5Bvalue%5D%5B%5D=0&components%5B23%5D%5Bvalue%5D%5B%5D=false&components%5B23%5D%5Bvalue%5D%5B%5D=false&components%5B24%5D%5Bkey%5D=js_fonts&components%5B24%5D%5Bvalue%5D%5B%5D=Arial&components%5B24%5D%5Bvalue%5D%5B%5D=Arial+Black&components%5B24%5D%5Bvalue%5D%5B%5D=Arial+Narrow&components%5B24%5D%5Bvalue%5D%5B%5D=Arial+Unicode+MS&components%5B24%5D%5Bvalue%5D%5B%5D=Book+Antiqua&components%5B24%5D%5Bvalue%5D%5B%5D=Bookman+Old+Style&components%5B24%5D%5Bvalue%5D%5B%5D=Calibri&components%5B24%5D%5Bvalue%5D%5B%5D=Cambria&components%5B24%5D%5Bvalue%5D%5B%5D=Cambria+Math&components%5B24%5D%5Bvalue%5D%5B%5D=Century&components%5B24%5D%5Bvalue%5D%5B%5D=Century+Gothic&components%5B24%5D%5Bvalue%5D%5B%5D=Century+Schoolbook&components%5B24%5D%5Bvalue%5D%5B%5D=Comic+Sans+MS&components%5B24%5D%5Bvalue%5D%5B%5D=Consolas&components%5B24%5D%5Bvalue%5D%5B%5D=Courier&components%5B24%5D%5Bvalue%5D%5B%5D=Courier+New&components%5B24%5D%5Bvalue%5D%5B%5D=Garamond&components%5B24%5D%5Bvalue%5D%5B%5D=Georgia&components%5B24%5D%5Bvalue%5D%5B%5D=Helvetica&components%5B24%5D%5Bvalue%5D%5B%5D=Impact&components%5B24%5D%5Bvalue%5D%5B%5D=Lucida+Bright&components%5B24%5D%5Bvalue%5D%5B%5D=Lucida+Calligraphy&components%5B24%5D%5Bvalue%5D%5B%5D=Lucida+Console&components%5B24%5D%5Bvalue%5D%5B%5D=Lucida+Fax&components%5B24%5D%5Bvalue%5D%5B%5D=Lucida+Handwriting&components%5B24%5D%5Bvalue%5D%5B%5D=Lucida+Sans&components%5B24%5D%5Bvalue%5D%5B%5D=Lucida+Sans+Typewriter&components%5B24%5D%5Bvalue%5D%5B%5D=Lucida+Sans+Unicode&components%5B24%5D%5Bvalue%5D%5B%5D=Microsoft+Sans+Serif&components%5B24%5D%5Bvalue%5D%5B%5D=Monotype+Corsiva&components%5B24%5D%5Bvalue%5D%5B%5D=MS+Gothic&components%5B24%5D%5Bvalue%5D%5B%5D=MS+PGothic&components%5B24%5D%5Bvalue%5D%5B%5D=MS+Reference+Sans+Serif&components%5B24%5D%5Bvalue%5D%5B%5D=MS+Sans+Serif&components%5B24%5D%5Bvalue%5D%5B%5D=MS+Serif&components%5B24%5D%5Bvalue%5D%5B%5D=Palatino+Linotype&components%5B24%5D%5Bvalue%5D%5B%5D=Segoe+Print&components%5B24%5D%5Bvalue%5D%5B%5D=Segoe+Script&components%5B24%5D%5Bvalue%5D%5B%5D=Segoe+UI&components%5B24%5D%5Bvalue%5D%5B%5D=Segoe+UI+Light&components%5B24%5D%5Bvalue%5D%5B%5D=Segoe+UI+Semibold&components%5B24%5D%5Bvalue%5D%5B%5D=Segoe+UI+Symbol&components%5B24%5D%5Bvalue%5D%5B%5D=Tahoma&components%5B24%5D%5Bvalue%5D%5B%5D=Times&components%5B24%5D%5Bvalue%5D%5B%5D=Times+New+Roman&components%5B24%5D%5Bvalue%5D%5B%5D=Trebuchet+MS&components%5B24%5D%5Bvalue%5D%5B%5D=Verdana&components%5B24%5D%5Bvalue%5D%5B%5D=Wingdings&components%5B24%5D%5Bvalue%5D%5B%5D=Wingdings+2&components%5B24%5D%5Bvalue%5D%5B%5D=Wingdings+3';
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