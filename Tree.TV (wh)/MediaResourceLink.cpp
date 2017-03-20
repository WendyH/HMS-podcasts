// VERSION = 2017.03.21
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
             'Cookie: mycook=a5e5915602d46d867444d612ae6e2897\r\n';
  if (!HmsRegExMatch('/player/(\\d+)', mpFilePath, sID)) return;
  // Загружаем страницу с фильмом, где устанавливаются кукисы UserEnter и key (они важны!)
  sHtml = HmsDownloadURL('http://tree.tv/player/'+sId+'/1', 'Referer: '+sHeaders, true);
  // Подтверждение своего Fingerprint (значения mycook в куках)
  sPost = 'result=a5e5915602d46d867444d612ae6e2897&components%5B0%5D%5Bkey%5D=user_agent&components%5B0%5D%5Bvalue%5D=Mozilla%2F5.0+(Windows+NT+6.1%3B+WOW64%3B+rv%3A51.0)+Gecko%2F20100101+Firefox%2F51.0&components%5B1%5D%5Bkey%5D=language&components%5B1%5D%5Bvalue%5D=ru-RU&components%5B2%5D%5Bkey%5D=color_depth&components%5B2%5D%5Bvalue%5D=24&components%5B3%5D%5Bkey%5D=pixel_ratio&components%5B3%5D%5Bvalue%5D=1&components%5B4%5D%5Bkey%5D=hardware_concurrency&components%5B4%5D%5Bvalue%5D=4&components%5B5%5D%5Bkey%5D=resolution&components%5B5%5D%5Bvalue%5D%5B%5D=1920&components%5B5%5D%5Bvalue%5D%5B%5D=1080&components%5B6%5D%5Bkey%5D=available_resolution&components%5B6%5D%5Bvalue%5D%5B%5D=1920&components%5B6%5D%5Bvalue%5D%5B%5D=1040&components%5B7%5D%5Bkey%5D=timezone_offset&components%5B7%5D%5Bvalue%5D=-360&components%5B8%5D%5Bkey%5D=session_storage&components%5B8%5D%5Bvalue%5D=1&components%5B9%5D%5Bkey%5D=local_storage&components%5B9%5D%5Bvalue%5D=1&components%5B10%5D%5Bkey%5D=indexed_db&components%5B10%5D%5Bvalue%5D=1&components%5B11%5D%5Bkey%5D=cpu_class&components%5B11%5D%5Bvalue%5D=unknown&components%5B12%5D%5Bkey%5D=navigator_platform&components%5B12%5D%5Bvalue%5D=Win32&components%5B13%5D%5Bkey%5D=do_not_track&components%5B13%5D%5Bvalue%5D=unspecified&components%5B14%5D%5Bkey%5D=regular_plugins&components%5B14%5D%5Bvalue%5D%5B%5D=Ace+Stream+P2P+Multimedia+Plug-in%3A%3AACE+Stream+Plug-in+Version+2.2.6%2C+Copyright+(c)+2012-2014+Innovative+Digital+Technologies%3A%3Aapplication%2Fx-acestream-plugin~&components%5B14%5D%5Bvalue%5D%5B%5D=Google+Update%3A%3AGoogle+Update%3A%3Aapplication%2Fx-vnd.google.update3webcontrol.3~%2Capplication%2Fx-vnd.google.oneclickctrl.9~&components%5B14%5D%5Bvalue%5D%5B%5D=Java+Deployment+Toolkit+8.0.1210.13%3A%3ANPRuntime+Script+Plug-in+Library+for+Java(TM)+Deploy%3A%3Aapplication%2Fjava-deployment-toolkit~&components%5B14%5D%5Bvalue%5D%5B%5D=Java(TM)+Platform+SE+8+U121%3A%3ANext+Generation+Java+Plug-in+11.121.2+for+Mozilla+browsers%3A%3Aapplication%2Fx-java-applet~%2Capplication%2Fx-java-bean~%2Capplication%2Fx-java-vm~%2Capplication%2Fx-java-applet%3Bversion%3D1.1.1~%2Capplication%2Fx-java-bean%3Bversion%3D1.1.1~%2Capplication%2Fx-java-applet%3Bversion%3D1.1~%2Capplication%2Fx-java-bean%3Bversion%3D1.1~%2Capplication%2Fx-java-applet%3Bversion%3D1.2~%2Capplication%2Fx-java-bean%3Bversion%3D1.2~%2Capplication%2Fx-java-applet%3Bversion%3D1.1.3~%2Capplication%2Fx-java-bean%3Bversion%3D1.1.3~%2Capplication%2Fx-java-applet%3Bversion%3D1.1.2~%2Capplication%2Fx-java-bean%3Bversion%3D1.1.2~%2Capplication%2Fx-java-applet%3Bversion%3D1.3~%2Capplication%2Fx-java-bean%3Bversion%3D1.3~%2Capplication%2Fx-java-applet%3Bversion%3D1.2.2~%2Capplication%2Fx-java-bean%3Bversion%3D1.2.2~%2Capplication%2Fx-java-applet%3Bversion%3D1.2.1~%2Capplication%2Fx-java-bean%3Bversion%3D1.2.1~%2Capplication%2Fx-java-applet%3Bversion%3D1.3.1~%2Capplication%2Fx-java-bean%3Bversion%3D1.3.1~%2Capplication%2Fx-java-applet%3Bversion%3D1.4~%2Capplication%2Fx-java-bean%3Bversion%3D1.4~%2Capplication%2Fx-java-applet%3Bversion%3D1.4.1~%2Capplication%2Fx-java-bean%3Bversion%3D1.4.1~%2Capplication%2Fx-java-applet%3Bversion%3D1.4.2~%2Capplication%2Fx-java-bean%3Bversion%3D1.4.2~%2Capplication%2Fx-java-applet%3Bversion%3D1.5~%2Capplication%2Fx-java-bean%3Bversion%3D1.5~%2Capplication%2Fx-java-applet%3Bversion%3D1.6~%2Capplication%2Fx-java-bean%3Bversion%3D1.6~%2Capplication%2Fx-java-applet%3Bversion%3D1.7~%2Capplication%2Fx-java-bean%3Bversion%3D1.7~%2Capplication%2Fx-java-applet%3Bversion%3D1.8~%2Capplication%2Fx-java-bean%3Bversion%3D1.8~%2Capplication%2Fx-java-applet%3Bjpi-version%3D1.8.0_121~%2Capplication%2Fx-java-bean%3Bjpi-version%3D1.8.0_121~%2Capplication%2Fx-java-vm-npruntime~%2Capplication%2Fx-java-applet%3Bdeploy%3D11.121.2~%2Capplication%2Fx-java-applet%3Bjavafx%3D8.0.121~&components%5B14%5D%5Bvalue%5D%5B%5D=Microsoft+Office+2010%3A%3AOffice+Authorization+plug-in+for+NPAPI+browsers%3A%3Aapplication%2Fx-msoffice14~*&components%5B14%5D%5Bvalue%5D%5B%5D=Microsoft+Office+2010%3A%3AThe+plug-in+allows+you+to+open+and+edit+files+using+Microsoft+Office+applications%3A%3Aapplication%2Fx-sharepoint~&components%5B14%5D%5Bvalue%5D%5B%5D=NVIDIA+3D+VISION%3A%3ANVIDIA+3D+Vision+Streaming+plugin+for+Mozilla+browsers%3A%3Aapplication%2Fmozilla-3dv-streaming-plugin~rts&components%5B14%5D%5Bvalue%5D%5B%5D=NVIDIA+3D+Vision%3A%3ANVIDIA+3D+Vision+plugin+for+Mozilla+browsers%3A%3Aimage%2Fjps~jps%2Cimage%2Fpns~pns%2Cimage%2Fmpo~mpo&components%5B14%5D%5Bvalue%5D%5B%5D=Silverlight+Plug-In%3A%3A5.1.50905.0%3A%3Aapplication%2Fx-silverlight~scr%2Capplication%2Fx-silverlight-2~&components%5B14%5D%5Bvalue%5D%5B%5D=VLC+Web+Plugin%3A%3AVLC+media+player+Web+Plugin%3A%3Aaudio%2Fmp1~%2Caudio%2Fmp2~%2Caudio%2Fmp3~%2Caudio%2Fmpeg~mp2%2Cmp3%2Cmpga%2Cmpega%2Caudio%2Fmpg~%2Caudio%2Fx-mp1~%2Caudio%2Fx-mp2~%2Caudio%2Fx-mp3~%2Caudio%2Fx-mpeg~mp2%2Cmp3%2Cmpga%2Cmpega%2Caudio%2Fx-mpg~%2Cvideo%2Fmpeg~mpg%2Cmpeg%2Cmpe%2Cvideo%2Fx-mpeg~mpg%2Cmpeg%2Cmpe%2Cvideo%2Fmp2t~%2Cvideo%2Fmpeg-system~mpg%2Cmpeg%2Cmpe%2Cvob%2Cvideo%2Fx-mpeg-system~mpg%2Cmpeg%2Cmpe%2Cvob%2Cvideo%2Fx-mpeg2~%2Caudio%2Faac~%2Caudio%2Fx-aac~%2Caudio%2Fmp4~aac%2Cmp4%2Cmpg4%2Caudio%2Fx-m4a~m4a%2Caudio%2Fm4a~m4a%2Cvideo%2Fmp4~mp4%2Cmpg4%2Capplication%2Fmpeg4-iod~mp4%2Cmpg4%2Capplication%2Fmpeg4-muxcodetable~mp4%2Cmpg4%2Capplication%2Fx-extension-m4a~%2Capplication%2Fx-extension-mp4~%2Cvideo%2Fx-m4v~m4v%2Cvideo%2Fmp4v-es~%2Caudio%2Fx-pn-windows-acm~%2Cvideo%2Fdivx~divx%2Cvideo%2Fmsvideo~avi%2Cvideo%2Fvnd.divx~%2Cvideo%2Fx-avi~%2Cvideo%2Fx-msvideo~avi%2Capplication%2Fogg~ogg%2Cvideo%2Fogg~ogv%2Caudio%2Fogg~oga%2Cogg%2Capplication%2Fx-ogg~ogg%2Cvideo%2Fx-ogm%2Bogg~%2Cvideo%2Fx-theora%2Bogg~%2Cvideo%2Fx-theora~%2Caudio%2Fx-vorbis%2Bogg~%2Caudio%2Fx-vorbis~%2Caudio%2Fx-speex~%2Caudio%2Fogg%3Bcodecs%3Dopus~opus%2Caudio%2Fopus~%2Capplication%2Fx-vlc-plugin~%2Caudio%2Fx-ms-asf~%2Caudio%2Fx-ms-asx~%2Caudio%2Fx-ms-wax~wax%2Cvideo%2Fx-ms-asf~asf%2Casx%2Cvideo%2Fx-ms-asf-plugin~%2Cvideo%2Fx-ms-asx~%2Cvideo%2Fx-ms-asf-plugin~asf%2Casx%2Cvideo%2Fx-ms-asf~asf%2Casx%2Capplication%2Fx-mplayer2~%2Cvideo%2Fx-ms-wm~wm%2Cvideo%2Fx-ms-wmv~wmv%2Cvideo%2Fx-ms-wmx~wmx%2Cvideo%2Fx-ms-wvx~wvx%2Caudio%2Fx-ms-wma~wma%2Capplication%2Fx-google-vlc-plugin~%2Caudio%2Fwav~wav%2Caudio%2Fx-wav~wav%2Caudio%2Fx-pn-wav~%2Caudio%2Fx-pn-au~%2Cvideo%2F3gp~%2Caudio%2F3gpp~3gp%2C3gpp%2Cvideo%2F3gpp~3gp%2C3gpp%2Caudio%2F3gpp2~3g2%2C3gpp2%2Cvideo%2F3gpp2~3g2%2C3gpp2%2Cvideo%2Ffli~fli%2Cvideo%2Fflv~flv%2Cvideo%2Fx-flc~%2Cvideo%2Fx-fli~%2Cvideo%2Fx-flv~flv%2Capplication%2Fx-matroska~mkv%2Cvideo%2Fx-matroska~mpv%2Cmkv%2Caudio%2Fx-matroska~mka%2Capplication%2Fxspf%2Bxml~xspf%2Caudio%2Fmpegurl~m3u%2Caudio%2Fx-mpegurl~m3u%2Caudio%2Fscpls~pls%2Caudio%2Fx-scpls~pls%2Ctext%2Fgoogle-video-pointer~%2Ctext%2Fx-google-video-pointer~%2Cvideo%2Fvnd.mpegurl~mxu%2Capplication%2Fvnd.apple.mpegurl~%2Capplication%2Fvnd.ms-asf~%2Capplication%2Fvnd.ms-wpl~%2Capplication%2Fsdp~%2Caudio%2Fdv~dif%2Cdv%2Cvideo%2Fdv~dif%2Cdv%2Caudio%2Fx-aiff~aif%2Caiff%2Caifc%2Caudio%2Fx-pn-aiff~%2Cvideo%2Fx-anim~%2Cvideo%2Fwebm~webm%2Caudio%2Fwebm~webm%2Capplication%2Fram~ram%2Capplication%2Fvnd.rn-realmedia-vbr~%2Capplication%2Fvnd.rn-realmedia~rm%2Caudio%2Fvnd.rn-realaudio~%2Caudio%2Fx-pn-realaudio-plugin~%2Caudio%2Fx-pn-realaudio~ra%2Crm%2Cram%2Caudio%2Fx-real-audio~%2Caudio%2Fx-realaudio~ra%2Cvideo%2Fvnd.rn-realvideo~%2Caudio%2Famr-wb~%2Caudio%2Famr~%2Caudio%2Famr-wb~awb%2Caudio%2Famr~amr%2Capplication%2Fx-flac~flac%2Caudio%2Fx-flac~flac%2Caudio%2Fflac~flac%2Capplication%2Fx-flash-video~%2Capplication%2Fx-shockwave-flash~swf%2Cswfl%2Caudio%2Fac3~%2Caudio%2Feac3~%2Caudio%2Fbasic~au%2Csnd%2Caudio%2Fmidi~mid%2Cmidi%2Ckar%2Caudio%2Fvnd.dts.hd~%2Caudio%2Fvnd.dolby.heaac.1~%2Caudio%2Fvnd.dolby.heaac.2~%2Caudio%2Fvnd.dolby.mlp~%2Caudio%2Fvnd.dts~%2Caudio%2Fx-ape~%2Caudio%2Fx-gsm~gsm%2Caudio%2Fx-musepack~%2Caudio%2Fx-shorten~%2Caudio%2Fx-tta~%2Caudio%2Fx-wavpack~%2Caudio%2Fx-it~%2Caudio%2Fx-mod~%2Caudio%2Fx-s3m~%2Caudio%2Fx-xm~%2Capplication%2Fmxf~mxf%2Cimage%2Fvnd.rn-realpix~%2Cmisc%2Fultravox~%2Cvideo%2Fx-nsv~&components%5B15%5D%5Bkey%5D=canvas&components%5B15%5D%5Bvalue%5D=canvas+winding%3Ayes~canvas+fp%3Adata%3Aimage%2Fpng%3Bbase64%2CiVBORw0KGgoAAAANSUhEUgAAB9AAAADICAYAAACwGnoBAAAgAElEQVR4nO3dfbicdXkv%2BltEQiUeKVgNlBY0lKYbqWmDBFu0tLJbPXX3pKe1la1nFzfJAEaQS90KFsRLN0Vbe9RTjxpooW3Unr5d6RY3u0jb8BKQwILFS0oMCSQESKKhBg2Y1LTe5481z%2BRZs55Za2bWzHpWks9nXfdFZuZ5m%2Bf58dd37t8vYpbLyJdl5JkZuTQjr8nIVRm5OiPXZ%2BTmjNyekdmsnc33Nja3uTEj%2FyAjz8vI0zNybtux52bkwox8W0ZekZE3ZORfN%2Fft9hyrm9d0Q0Ze1TzWwow8cvwXyZdF5pmRuTQyr4nMVZG5OjLXR%2BbmyNy%2B%2FxS5s%2FnexuY2N0bmH0TmeZF5euT47wEAAAAAAADAQSgjT8vId2bk5zLynlJwPYja1wzDt2bktwd87HH1UGT%2Bvy%2FNb%2F7motxyUiO3x19kxvaBHX5vZN4VmZ%2BNzHdG5mt6useNyEOxhjVmAQAAAAAAAAYmI09qdm%2BvH2aoPezaHJkficwFkRlVdXhmLM6M5ZlxfWY8ONDTr4%2BxLvVXT3m%2FZ0GYfbAE6PnYBTfmYxfkWDWuGsY5AAAAAAAAgINcjk2fvjTHpkCvPfzut3ZH5nWReXZ0CM2nqgWZcWlm3DvQy7o3Mi%2BNzJdV3vtZEGYf6AH6%2FtC8qi68Z5DnAgAAAAAAAA5SGfmajPxoRj7STRJ8ddyfESsyYkUuiZszYkWujEcHkjKvjEczYkXr9fGxMpfHmq72fSAyr4zMn5osHI%2FMiFUZcXN3YfovZcY1mTHS4bSNFRk33d%2FLV9wSmX8Yma8rP4MljYvyjMbltQfaM1nHNz6R0Vjx2WmP38cuOH3y8LxVT073XAAAAAAAAMBssOzatdFYka264Auvi8aKVdM5ZEaenpHXZOTGXgLuM2LVuCD9rtgx0PC8CNDPiFUZsWLKAP3eyLwsMk%2FuJhBvBv5dB%2BhFHZ4Zb8qMv8iMPc1Tf2BlPwF6UU9H5mci8w3RWPHZaKw45AL0QXWgdxme60QHAAAAAACAA95YUJ6x7Nq1495vrHi6FaT3KCN%2FLCOvzMjHek19l8eaXBI3DyQwn24H%2BtbI%2FGhkvqrbELxVPXSgV9WizLg6M9bndAL0or4ZmZ944yztQL%2Br8apc0rho1gbo5fXO7%2F3bX88VHz5rQmj%2Bwf%2Frp9vesy46AAAAAAAAHJjGOs%2BfrvysseKzccEX3tHL4TLy7Rl5S79p75K4eVYE6F%2BMzHP6Cb9jAAF6Uadmxs%2BuyPjstAL0jMw8pnFRntS4PPfOgtC8XEsaF832AP3JIhhv%2FNpPZuPXfjJvvu5%2Fb4XlKz58VjZ%2B7Sfz45e8thyi3zjd8wIAAAAAAAAz7YIvvKM5ZXvndaKLDvWiS739dcRY0H7eivy5H%2F27J5fHmjw%2BVo6bLn1lPDrhdVXKW3xe1NVxf94VOybss6Q1TfqKPD5Wtt4vti0%2BL6aCL597eaypDNCPj5UZsSLnxcp8WyvEvr%2Ftmu5vvr%2Bj%2BfrRZli%2BovnfqgB9TWn%2FHR3C8hUV9ej%2Bz467P%2BMtazKWNO%2F7524eu%2Fyb7h97%2FXurxl4X3erF1O8fWLn%2F9jYuymhcnvMbb25N07%2BysbjVBR6NFXl84xO5srE4r268eWIQXZre%2F67GqzIbrTXGW8c5o3F5a5vljXNbxy2OXX5drMleddxO5z6%2B8Ym8uvHmXNlY3Nq3CN%2BXNC4a97q034eaY3TVuCUKippE%2B9rnRVhefu%2Fev%2F31CaG6tdABAAAAAADgQNRY8aHmNO1Td5m3bzc2xftnIyIy8lfedsw%2FbL26FDgX4XV7mL0kbm79u5sO9PbQfXmsGbf%2F8bGyFaIX25b3b%2B84L9Y9L%2B9frLP%2B95E5J1aWwvCVpSD75ubr9sD70bZAvT1ALx%2BjU7f6mua%2FH207TnGuZnD%2Fsvszfm5lxuOlW1aE6UVo3liR8diO%2FYF68XkzYI7GRbm4EXlGqfP7jMbleVfjVa2AuypALwLz8md3NV41Ljwv%2Fr2ysXhcsF4Ots9oXD7uGGc0Lp%2B0A718Pe3Hbd%2B33M1ehPulAP2zpbHb1bhvD9B7KAE6AAAAAAAAHHB6C9BXRWPFqtLrjMaKpzPyvC%2FM%2Becnr477WwF3uSt8qtdTBejtHejt3ehFQL4yHu3YrV6eor3TFO43ROarx4XY7UH3%2FaUAvT0wz6wO0CfrOi9qZamzver1iomf%2F4c1GX%2BeGXc%2FOn599Pb10r%2B8Zn8XerMDPRqR0Yh8SePcPLG5JvpUIXZRyxvnjtuuCLbLneXlWt44d8Ln7eH8ZOde3jh3wrrt3QboEzrQC%2FtnUPhQ%2BxBv1x6gF1O4V1XbFO4CdAAAAAAAADjg7J%2FCfcowsRm2P138%2B6jz%2FuS8OGNFXv%2FCb%2BwuB9TDDNCrAvIiVO803fvxsTKLcL8qQD8uVuaZsSaPbQXUVQH6yuZ7vQTo7dO%2Bd6o1pY734rg7snOA3gzyD8%2BMn7854%2BvZOUAvpnOvCNCjcW4e1rg8f7cR%2BbW2qdY7BehFEF5Mtd4eoHeagr3YttcAvWp99GkH6GMzJ6yqGuJVijXQN6%2F%2Bz%2FlPX%2F1idvKVlR%2B3BjoAAAAAAAAc8MYCxac7fPahcQFks1v9R%2F7PP%2F%2BzjPzMGbEql8eanKkAvQjLy%2Bcrd6V3CtA7daA%2FEZlHxcrSFOrtnearSoF2Px3oj2Z3XehV66x3CtDL792ccXRmfCoz9nUI0Fvro08M0IvXSxuR60uhc3vXd3vgvbxxbl7deHMrMC8C9CLYLt4rOtCLNcuLEL283VQB%2BkA70MdmUage6x3kYxf8cfsa6FU1bg30TY3f6fP%2FRgAAAAAAAKBW%2B7vQxweLY%2BH5%2BE7dZdeufcWb%2F%2FzWdx%2B75uEiDG8PtIcdoBdrqt8VOzIj8%2Bq4v7UmelWAXlxj%2B%2F4RK%2FKtre7ym0vhdDGFens3eBGgP5qd1yqvWgO9vHZ6Va2aImCvCtDXTDz%2F2zNjSWnN82Jd9CJQnyRAj8bl%2BfONV%2BWtpbB7sk7y4xufaIXj5SA8GivGTb%2B%2BsrE472q8atzxljfOHdetXoTeKxuLxwXr5TXPy13r5QC9HLCXp4kvQvQJa6CP%2FQDkdaUx3lUnetGFno9dkB%2B%2F5LUTwvO%2F%2FMM36j4HAAAAAACAg8pYJ3p5DesJ4eKCN%2F3lZ%2BKo%2FdOftwfWy1vB7opW5%2Fdkr9vD8%2FGd2JOva15s0x6el%2Fet2r749wnjAumVpX3LYXV5KvaVbf8t6tEJ172%2Fbi6F3Z2mc7%2B5Yr8icO%2FUmb6jOpR%2F%2BYqM31q1%2Fxl%2Bec3YLfjczaXnenkzPC%2B%2Fviij8YnWe5NNxV4E0%2B1TsWcj8vjSMYqAvbz2efta6Ssbi1td6Z1C%2B%2BXjrnX%2Ffu2h%2BfGNT4zrQJ8wlivWaI%2FGiqdLa6JP2pleDtEnKeE5AAAAAAAAHAoy8syMvKNT5%2FiwqgjGi47z6dbXI%2FOsjt3eddSaiveKLvdO%2B%2BzosN%2BKjJfcn3FVZvxL21dvdZ5PXosakasnCc%2FrrvYp4KfcvluNFZ8d151e9f%2FAYxfc2Dk8b1w17f%2FJAAAAAAAAgNkvI0%2FKyL%2BZ6fC8CNAnm%2FK9l9ocmb8xkNB7UFVeV71cN0%2Bx381ZPe17qVP97Zlxd%2B8BejQi39KI%2FMYsCMtnLEAf60D%2F7JD%2FNwIAAAAAAAAOdBn5Qxn52ZkOzo8vTZU%2BiO7z70Xm8oGG34Oq8jTx5Wnh27crT1FfNRV8xXTv8zJjtPcAPRqRyxqR35kFgXl7eN4%2BjfvAOtABAAAAAAAAppKRV9TReT7o%2BtiMBeKzrE7NjC%2F2HqBHI%2FLDsyA0n3boDgAAAAAAADAIGbksI5%2BtO%2Fyebl0bmS%2BtM8Suu348M87qPUB%2FcSPyc7MgBBegAwAAAAAAALXKyP%2BUkRvqDr%2BnW1%2BJzFPqDrBnQ82JjDN7D9HnNyJXzYIgXIAOAAAAAAAA1CIjz8jI2%2BoOv6dbayPzDXUH17OpXpQZH86MPT3fyjsic3Hd4xIAAAAAAABgxmXk%2F6o7%2FB5EvanuwHq21pV93c5b6h6XAAAAAAAAADMqI99Zd%2FA9iLq%2B7pB6NtcLM%2BPjfd3WZXWPTwAAAAAAAIAZkZHz8yCYun1TmLp9yvrfMuOPer61d0bmKXWPUzjU5Prlx7ZqW%2BPFdV8PAAAAAADAISEjP1Z3%2BD2IuqLucPpAqeMy4%2Fqeb%2B%2Fv1T1O4WCX2XhRPrX0hNy47NTcdNGiCbV52cLcdP7JuX75sXVfKwAAAAAAwEEpI%2Bdl5L66w%2B%2Fp1vbIPLzuYPpAqvmZ8f%2F1dIv3RebJdY9XOFjlpgtfPhaQVwTnVfXYslMyGy%2Bq%2B7oBAAAAAAAOKhn5h3WH34Oo99YdSB%2BIdXRmbO7pNv8%2FdY9XDlwX3xRz6r6G2So3nX9y18F5e0f6Y42X1n39AAAAAAAAB4WMPD0jt5QT0ufimfxWbMqn4qHcEiO5JUZya4zmjtiQz8UztQflVXVvZJ7YV4i8LSPWZcRIs9ZnxJbSv%2BsOuauuZcDX9%2BuZ8VTXt%2FrpyPy5ycbU%2BXfGiUvvi0WNkVgwU%2BO4bPn6OLau8xff%2Ffw748SZPvdstnx9HNsYidOWr49ZOe340vti0dL7YlFd5%2B87PBeiAwAAAAAADFZGfnz%2F%2FNx7c3usb4XmnepbsSn3xd7aQ%2FNyfbCv8PiZUnB%2BCAfokRkf7Ol2%2F%2BFkY0qALkBv1xiJBUvvi0UC9InyqaUnTCs8b03n3jjNdO4AAAAAAADTkJEvy8g9RXi%2BNUZb3ebfjidzb%2BxuJabPx658Jra0QvQdsaH20LyonZF55LTC6YcyYu8sCMtrDNBfnBl%2F0vUt3xOZ8zqNKwG6AL3dbA%2FQ65LbGi%2Fuac3zqWrL0lfW%2FZ0AAAAAAAAOWBn5niIRLTrPn451k3aXPxvbWiH6d2JH7eF5RuanBxpOz7aaoQA9MmNhZtze9W1%2FX6dxJUAXoLcToFermrp91ed%2BedE1lyxedMeXllSG5Dvv%2B51F11151qLrrjxr0UNffevEbbY1Xlz39wIAAAAAADggZeSNGWNrnhed591Mzf6t2NTqQi93rnfad2c8nt%2BKTZWfFed%2BOtZlOch%2FJrbk87Erd8SGVmC%2FPdbn87Eri475nfF4bo3RfGOri3xLl2Fx1dTtRU0VUO9tfl5eN31dRjw5RQD%2BTEY8Xtqn%2Fdg72o65ISN2dRGg782ITV1eS6frL461LeO3Msf9LmLvloznR8b%2B%2B2%2B7MvZsyHh%2BJI%2F87j%2Fe2RiJ06qC4skC9MZIHLf0vli0bDQWnjcaR3c7VpfeHa9Ydk%2BcWky1vWxtnHLeaBxdda72AP3im2JOsd%2FFN8WcquO%2Fa3XMLa6rdc7mPsvXx7Gdzt%2Fpu59%2FZ5z4rtUxtzES85eNxsLm9VTer9Y1ro1XNkbitNY57olTGyNxXLf3qAipJ9un9R3ujleU31%2B%2BPo5dtjZOKa512WgsrPqOF98Uc4prbIzE%2FPbjnzcaRxf7v2t1zC3uR3t1%2BwODi2%2BKOeffGScW51w2GguXro1XNq9jQfuxpvoBQ9U%2BEROncO903f1%2Bj07au8%2Fv%2BNKSRUuXLGhVVUD%2BmcsWtz6%2FcunPTgzQn1p6wnSuCQAAAAAA4JCUkW%2FIyL3lQLxTyN1e%2B2JvK8gu7%2F9sbKvc%2Fql4KLfG6KRhfLFvEaDviA2tYL5cW2M0n49d%2BXSsyy0xkn8VI3nEuCC4mxC93wB9dzOo77Tvupw4FXxxrA1t224obbOpw%2FFGS%2FtVBejrJrmedRXfe6rrH8mIxzM%2BVhGgf2%2Fd2H9bde%2F3T9l25dIiTC6PrU4Bet%2Fh%2Bdp4ZVV4WYS8UwXoERGl7SrD5aV3xwntoXBxnsZIzO8YorYF0cV3L4fR7dUePF98U8wph%2FNTbd9J6%2F62PY9C1Y8EyvtNcv5x96y4v1Ud5aWA%2FoTy%2FegneJ7svjR%2FXFB7gF58z37k%2BuXHVnWflwP0%2B1ZN7EK%2F5pL9Afolbz1tYoC%2BuVHLzA8AAAAAAAAHtIz8WJGSFmF0v1OyPx%2B7Oq6L%2Fp3YMemU71tjdFznexGgFx3nxTrsz8Uz49Zo3xqj%2BZ3YkVe0uqqLEHq0iwC9qpO7m%2FfXlc6xo%2FT%2BtuZ77cF4%2BVgjzWvc26zdub%2FzvBz%2B7y29P1r6rCpAL66l6Djf2%2Fz3aOl45Wspwvh1OdbdXry%2FK8d1pM%2Ffu38q9yJAL0L0f3tm7P1%2F35tzd9%2F2mSJILAfiVQF6v%2BF5Oaw9%2F844seggX3p3vKIcUE8VoC%2B9O15RBK9V56nqzG4PbIuwuNlZvqD4PuWu9nLwuuyeOLU4XrNjuhXEv2t1zC1d2wnF9uV7U7xfFVRXufimmFPck6pO%2B6ofCbTf3%2BK62rvH259Z8aOGxkicVpxripkHep7CvTjHstFYWIT4RUd6pzB%2BUAH6ZAa1REE%2BtfSEqunZr1z6s4uWLlmw6JpLFldO4X7Hl5YsuuStpy1aumTBopuv%2B9WKAH3ZwqnPDgAAAAAAwDgZub4IsYvA%2Brl4pq8AvRx8t0%2Fj%2Fq3Y1PqsvcO9CNfL7xfbPhUPTTjWM7FlwrUuaIXAe0uh8q6cOjzvNUDfMcXxy13tuyuO1SnYX98h7C6C7akC9B0V%2Bz1Zcc7y%2Fdldsc%2Fu0ufPZLyzIkD%2F973tj319aTrvVpdye8DYb3gesb9zfOnaeGX7Z8V04d0E6M3rOK09vI7o3Jld7nRvD6TLU5mXu5DL4W77ecrXUA7qO4W6zc%2Fm9zJVeCukr%2BiMLs5dDrFb97dDJ3XpWU7omm99l7Xxyvap2yvO3VOAXp52v2qf8n2eyQC9NZbviVM7LQfQrXx8%2BYlVAfkgajrXBQAAAAAAcMjJyFcX07cPKkB%2FNrbllhjJb8eT497fGqP57Xgyt8f6CdO4F9O3l6eDLwL0nfH4hHOU12rPyHw4MueMC4FLAfDAA%2FRi%2FfJNkxyv6OJ%2BsotztF9zVaidOfUU7p2upehC7%2FZetN2%2FF2TGn5YC9D3rqx77nlOf%2Ft3%2F1CnEbIzEgumE5xH7g82qUDZi3NTsUwboRUfzhPC0ojO7fO5OYWxpv9Z5pupOrgpwy53WS%2B%2BOEzp9124U3729074IuBsjcVrVd%2Bx0vCLIbv9xQflcrW77SYL4XgP0YsaA9uutuvaZCtBbsxhU%2FKCiHwJ0AAAAAACAWSIj%2F0s5BR1EgF6E5U%2FHutbrosN8b%2BzOnfH4uGnc98Xe3BIj47YvB%2BjPxJaOAfr2WJ8ZmX82WQA88AB9sk7x9v22TPFeUeWu9V6usXhvsjC%2FuN4nO3y%2Bq3n%2BLc3jlNdGb96%2FszPjG80Afe%2BWysf%2BI9%2F%2B6w9NEqCfVkwp3qkzeTLlgLbTNlWBdacAveg0bw9lq6Zvj9gfqnYKfVthdSlc7ifAfdfqmNu%2BXnpjJE4rOrs7ffdOqjrtW%2BvIlzr5y%2Fe3m6o6V3l9%2BsmmNO81QO9mmvRhroHe7rzROHrZaCzs94cgVaqmcH%2Foq29ddN2VZy265pLFiy77ndPGrYdermsuWbzomksWL1r1uV82hTsAAAAAAMB0ZeSnq0LrftdAL6oIyYt1y78Vm%2FKpeGhcmF5M1150rD8b2%2FoO0N9zyAfok11L1fWW14qfrEr373cnD9CP2n37lzuFmKX1qxf0s2b0oAP0iIlBbqfp2yO6D9DL19dvgHveaBzdmk69rdrXRp9KVVheBPTlUH0QAfq4LvS1cUqnazqQA%2FQiPO%2Fl%2BruR65cf2x5%2B%2F%2Bk1v7joq3%2F1uUU7tz2x6Plnn12UmZW1c9sTi554Yv2ipUsWLNp53%2B%2B0BeiNaa3NDgAAAAAAcMjJ0vrn5eC7fY3yqbrNt8f6Vliekbk3dremcd8Xe3NrjI6bin1rjLamX98RG3JrjE5Y57yXAH3%2F%2BueHaoD%2BeBcB%2BrbSe%2BtK51uXY9PDb2lu07YGerHPkVsyHu8coB%2F2vYee6hRiFt264zqs27q8JzOkAP248nTtnaZvj%2Bg%2BQC93tPcb4Baaa4sf1xiJ%2Ba01xqeYyrxde6d9a%2Brxtmndu7m%2FUym696d6vgdqgH7xTTGn%2BI6NkTium2vvRW5etrAcfv%2FVJ%2FcH6FNVEaBP6EB%2FamnlNPoAAAAAAABUyMh5Gbm7nII%2BH7ta07iXA%2FFO9e14ctxa5O0B%2BPZY3%2Bo4L08LvyM2tN7rFNh3G6Bvj8y5MxqgT3cN9E7B%2B1RroBcd4%2F2ugb6r%2BXpb8%2FVDHc61t8P925Lxns4BeuxZn2c%2B%2FrZf67QGevt7va4fXQSbg1gDPaIZiDan4o7YHwBXdXi3uq87rOtdhO%2FlzuvpBujtyiF3L93P5e%2FVGIn5Vd%2BjWN98svs7mdYzXRuntO5Fh%2Bfba4Be9eOEimOe1nH2g1L3fVlxX7oJ0MvhebfPq1e56fyTy%2BH3E%2F%2F0nztO215V11151sQAfVvjxcO4VgAAAAAAgINSRr6%2BKgktguunY92ErvD2sH1rjHYMuYup2XfG4xMC9uKz4lzPx66%2BA%2FTbJw2jhxGg7ygdf1fFsZ7p8PlUAfqG5ucbKj4rd4VXBeidruXJUlg%2B1Xdt%2F4FARYD%2BypGMtZ0D9JO3f%2ByiqQL0iP3hZVW3dydFQF4VYBad1r0E6BERRaBc6kavDGkn6%2F5udoqf1t6Z3E%2BA3uo079C93U%2BAXny3pWvjlVXTtxdaHeQdpl8vutfb70F5%2FfciMC%2B%2BW9Wxeg3QI%2FZPO1%2F1A4bW9%2Bs0%2B0Fbt33E2FTsVftEVAfopXHS9XjtVW5rvLiqC72b8PySt542cfr2LUsrfzgAAAAAAABABxn5zqokdG%2FsbgXjW2M0n4kt4wLu52NXa6r3ImifbHr3qg7zfbF3yv27DdCvn%2FEAPXN%2Fh%2FloM1Av3t%2BW%2Bzu%2B24PwqQL0cvD%2BeO7vDn%2BmGYBPFaCXr2Vv22flTvhtpe3L07rvyv0hfocAPUYyPtQ5QH%2FFM3%2F837sJ0Pvppi7vc%2F6dcWIR1i5fH8e2TXHedYBe7m6erFt53Drka%2BOUIoA%2BbzSOLv0YYFyw3E%2BAXu7OL4fo71odc0shbtdTuEeM7y6fLCBvX8O86MQvppKvCrHLPx5ov95S1%2F64HwP02nkfUerwH7svJxTPfundcULrutqOWQT%2B7eNl3HfpIkAv3fehryeeTy09ob2LfKoQ%2FcqlP7vooa%2B%2BdXx4%2FljjtMzGi4Z9vQAAAAAAAAeVjLx6su7yp2NdK%2BTuVNtj%2FaRd6kXQ%2Fp3Y0TEgfza2TStA%2F1AtAfrutlC7vdY1Q%2BxeAvRyuN1eozn1FO7rOuzbPtX83km2LcL7qnXem%2Bf6qS3ZXHp%2BQoD%2B0l1%2Fe303AXrEuGCy60B46dp4ZTkMbgW%2Bo7GwqkN9qgC9eR2t8L1q%2BvaI%2FaFqEZZXnb99334C9PJU4R2rh7XjS%2BeaX%2FqBQcf1u8vd3FXV3oFdPI%2BqUL7TVP3la%2BklSG%2Ffb9yz7zC9esfndU%2Bc2s0a6OUfFUxVVcsUtHeyd6N9KvdiOvdVn%2FvlRZ%2B5bPGiay4Zqz%2B95hcX3fGlJROnbd%2B8bGE%2B1nhpr%2BcFAAAAAAA45GXkqqnWOH82tuWO2NDqJC%2B60nfEhnFrmneqvbE7t8RIZcj%2B7Xgyt8ZoxwC%2B2wB9SS0BernLuxykr8vx3d69BujZvOZyJ%2FiGHOsOr7qW8nt7SyF7cS3bOpyj2Ha07TzPtB13Q8W5tmRcXR2gH%2FndW1Z3G6AXa5D30Y38inIwWnRLVwXW3QToRXfzZEF%2BuVt%2B6d1xQmnK9tMaIzG%2Faq3vftdAv%2FimmHP%2BnXHiuO84GgsbIzG%2Fn%2FXJm9%2FxFVVhdpXl6%2BPYZWvjlHKXdmMkFrQH71VTt1d8x9PaA%2FaLb4o5xY8dJuuI7%2FQ9yveluPdT3cvy8yq60WdrgB5RHaJ3VcJzAAAAAACA%2FmXkmqkC8AOhfr6rgLzfmmq98EO05mXGnsrHcUdd47noUO4ljI8odbV3mL49or%2B1x5k5%2FUwLP2xF%2BN7v%2Frnpwpe3r4k%2BaT227BTTtgMAAAAAAExDRo7WHX4PohYONSwWoHes6yofx%2BiwxutkU5iXu9l7neK86E6erLtbgD67zcYAvTESx%2FW6Xn27zMaL8qmlJ%2BTGZad27DjfdP7JuX65cQkAAAAAADBdGbmp7vB7EDV%2FKAFxsX551VrgKiIzfrvycWwc1ngtpv1edk%2BcWg7JzxuNo4tpvbsJLIvpxpvTeM%2FvZhpxAfrsNtsC9GIt%2B0FfT65ffmyrtjVePMhjAwAAAAAAHPIycnvd4fcgat5QAuJ1uX9t8F7WUj%2BE6tjMuH%2FC49g%2BrPF63mgcXV6Xu72WjcbC80bj6KmO867VMbfX%2FQTos9tsC9DftTrmNkZiwVTrzQMAAAAAADCLZOTuusPvQdTcoQTEo83g%2FKGM2FZ%2FWD1b6%2FcnPI7dwxyzzWByfjHtetF13hiJ%2BZNNwd6u2L8xEqd1E4oL0Ge32RagAwAAAAAAcAASoGdG7MyIr2fEdRlxWUYsyYizM2JBRpyUEfMyIpr1suZ7Jze3eUtGvD8jbsiIezNid%2F2B9kzXwgmP47m6xzUAAAAAAABAz%2FKQnML9oYy4PiMuyojXlsLxQdScjHhdRixvnuOB%2BgPumaiHxz2OHTM8jAEAAAAAAACmLyM31R1%2BD6LmTxnybs6Ij%2BRYV%2FkgA%2FNuakGOdak%2FXH%2FQPay6Ytzj2DQzoxcAAAAAAABggDLygbrD70HUayqD3d05Ni372TWE5p3q9Iz4VI5NGz8Lgu9B1fhp3B8Y9rgFAAAAAAAAGLiMXFN3%2BD2I%2Bvlxge4DGXFlRvzULAjMO9WJGfHejLir%2FvB7ULW99TjWDHPMAgAAAAAAAAxFRq6qO%2FweRC2JzIh7M%2BKyjDh5FgTk3dbxGXFJRtxWfwA%2B3bqh9ThuHNZ4BQAAAAAAABiajPx43eH39GtrfjA%2BmhGvmgWBeL%2F18oxf%2BEDGTz1SfxDeb72r9Uh%2BfxhjFQAAAAAAAGCoMvKC%2BgPw6dQXM%2BOc%2FELtAfgA6nOR8eSZGW%2F7QkbsrT8Q77VOaj2WiwY%2FUgEAAAAAAACGLCP%2FY%2F0heD%2B1KzPe1kpvv1Z3%2BD2I%2BvvI1t8t5z3XPuYAABRkSURBVGSc8GT9oXivtTkzMn9l8CMVAAAAAAAAYMgycn79YXiv9feZcc645HZT3eH3IGpDKUDPyNi%2BOOPMv64%2FFO%2BlbsiMzFMGP1IBAAAAAAAAZkBGPlt%2FKN5t3ZAZr65Mb0%2BqOwCfTs1rC8%2BLvy0nZrz7j%2BoPxrusl16Qzw9hiAIAAAAAAADMjIx8rP5gfKr618z4vcw4tmN626g7BJ9Ond8hQM%2FI2HNkxn%2F%2F3Yyjd9UekE9V%2F8fpuXUYYxQAAAAAAABgRmTkE%2FUH5JPVE5lxyZTp7XV1h%2BDTqc9PEqAXf9ctzViwvvaQfLL65LG5bRhjFAAAAAAAAGDoMvJHMvK5%2BkPyTnV3Zry1q%2FR2NCJfVHcQ3k%2B9IDJGugjQMzJufEvGL9xae1BeVS%2BIzHsPy3%2FNyOOGM1oBAAAAAAAAhigjX11%2FSN6pHs6ME3pKcRfUHYb3U3O7DM%2BLv%2B3zMs66o%2FbAvL1O2v%2FsFg5ntAIAAAAAAAAMUUb%2BZv1BeVV9PTPO6jnFvaLuMLyfOiwynuw1RF%2BUcdLq2kPzcl22%2F%2Fm9bTijFQAAAAAAAGCIMvKq%2BsPy9tqcGb%2FRV4p7W0TOqTsQ76f%2B7x4D9Iyx6dx%2F8hu1B%2BcRmYdF5j%2Fuf4YfHs5oBQAAAAAAABiijLyh%2FsC8XN%2FLjOXTSnPfUncY3k%2Bd00eAnpGxallGfKf2AP1Xxj%2FHG4YzWgEAAAAAAACGKCP%2Fpv7QvFwfm3aa%2B%2Bm6w%2FB%2B6uWRsaHPEP2jH649QP%2Fk%2BOf418MZrQAAAAAAAABDlJF%2FX39oXtS1mfHSaae5OyPyyLoD8X7qT%2FoM0J9%2FccZFn6stPD8yMneOf5b%2FazijFQAAAAAAAGCIMnJN%2FcF5ZsZXMuOUgaW6l9UdhvdT7%2B8zQM%2FI2DQ%2FY8mqWgL09058nncMZ7QCAAAAAAAADFFGPlB%2FeL42M94w0FT33og8se5AvNf6L9MI0DMy7jgr48y7ZzQ8Pz4y75r4TEeHNFwBAAAAAAAAhicjN9cfoL9pKOnu%2B%2BoOxHuts6cZoGdk3HLOjAboF1c%2F081DGq4AAAAAAAAAw5ORO%2BoNz68fWrq7PSIPrzsU76VeM4AAPSNj2bUzEp4fHpkbq5%2Fr9mGNVwAAAAAAAIChycjn6gvPN%2BWgp25vryvqDsV7qeMGFKDf%2BXMZp2wYeoD%2Bgc7PdvewxisAAAAAAADA0NQXnmdmXDHchDciN0XkG%2BoOxrutuZFx6YCOddnlw358X4%2FMBR3G1KX9jUYAAAAAAACAGmXk9nrC8%2B2ZcfjQA%2FSMyOvrDsa7rZObHeSDCNEPPzxj48ZhPsILOoynSzNyZ5%2FDEQAAAAAAAKA%2BGTnUlLVzvXdGwvOi3lR3ON5NndMM0PdExmUDON7FFw%2Fr8d3SYSxdlpF7MnJTv%2BMRAAAAAAAAoDYZ%2BcDMh%2Bf3ZsaJMxqgr43I19cdkE9Vb28G6IMK0Y8%2FPuPOOwf9%2BNZG5usrxlERnmdGPjC9UQkAAAAAAABQg4xcM%2FMB%2BgdnNDwv6u8i8ifqDsknq%2FeWAvRBhejvfe8gH93WyPytijFUDs8zI9dMc1gCAAAAAAAAzLyMXD2z4fnOzDiylgA9I%2FILETm37qC8U61sC9CLv%2BmsiX7kkRnbB7LM%2Fb7IfE%2FF%2BLm0YuPV0x6YAAAAAAAAADMtI%2F9mZgP0T9cWnhd1Vd1BeVUdExn%2F3CFAn24n%2Bic%2FOYhH9%2FGKsdPeeV7U305%2FZAIAAAAAAADMsIy8YWYD9LfUHqB%2FNyIbdQfm7XV2h%2FC8%2FNdvJ%2Frpb57uY%2FuzyPyRtnFT1Xle1A2DGp8AAAAAAAAAMyYjPzJz4fltmTGn9gA9I%2FLRiPy1ukPzcv1%2BFwF6v53oLzoi47bb%2B31sX4vMn24bM506z4v66OBGKAAAAAAAAMAMyci3zVyAfkXtwXm5bovI19YdnBe1sYsAvfjrpxP9iqv6eWQPR%2BavtI2XyTrPi3rHAIcoAAAAAAAAwMzIyNNnLkBfUHto3l53ROS8usPzt%2FUQnvcboi9Y0Ovj2h2ZZ7WNlW7C88zI0wc7SgEAAAAAAABmQEaekJH%2FOvzw%2FOGcLdO3t9fqiHxLXeH5j0bGrX0E6L1O537kkRkPP9zt4%2FpaTOw8n2ra9qL2ZeSPDXygAgAAAAAAAMyEjPzG8AP0P6s9KJ%2BsvhGRy%2BoI0N%2FfR3jeb4h%2B%2FfXdPKo%2Fj97XPC%2FXhsGPUAAAAAAAAIAZkpE3DT9Af0%2FtIflU9Z2I%2FHBEHjVT4flPRcb90wjQew3RL7poskf0b5H5ich8edvY6CU8z4z8%2B%2BGMUgAAAAAAAIAZkJGfH36APvvWP%2B9Un4%2FIk2ciQP%2BLaYbn5b9u1kQ%2F6aROj%2BfJyLy0Ylx0u%2BZ5uT4%2FjDEKAAAAAAAAMCMy8sLhhufbM2Nu7cF4L%2FV3Efn6YYbnHxlgeN5LiL7zsfbHc29k%2FnbFmOgnPM%2BMvHA4oxQAAAAAAABgBmTk4uEG6LfXHoj3U9sj8k3DCM8viIzdQwjQu5nO%2FR%2F%2Bofxo7ojMkyrGQ6%2FTtpfr54Y1TgEAAAAAAABmREZuHl6Afn3tYXi%2F9YOIvD4i3zCo8PysIYXn5b%2FJOtGvuzZjrOt8eWQeVTEO%2Bu08z4zcPtRBCgAAAAAAADATcpjroP%2FkhVvy8PrD8OnUnoi8IiIPn054fkFkbBxyeJ7RsRP98Ig8993v3hGZR3cYA9PpPM%2BMvG64oxQAAAAAAABgBmTkO4YWoK846e68KzLfF5kn1h%2BGT6fuisj3ReSJvYbnH5mB4Lz9r9mJ%2FiMReWFE3hKR%2BfrXb%2Bvw%2FKfTeV7UeTMwVAEAAAAAAACGKyPnDS1A%2F8oxD7de3BuZH4zMk%2BsPw6dT90bkByPy5KmC86Mj4y9qCM8z8viMvPjSyFvL1376wmcrnv0gwvPMmLieOgAAAAAAAMABKSPvGEqAfvdR35jw5s7I%2FHxknll%2FGD6d2hmRn4%2FIM9uD8x%2BNjP8WGaMzH5wvzMg%2FyMgnMzL3ROZlpWtesOB7bc98utO2F%2FX1WgYtAAAAAAAAwDBk5KeGEqBvfNGWSTfYGJlXROaCGkLwl0bmYYM51sYYWyd9wdtiZtY6L%2F0tyMgrMvLhTvf40uZ1nnDS90vPe1Cd55mRn6pz7AIAAAAAAAAMVI41Lw8%2BQH%2F6sG92vfFoZF4XmY3I%2FJkhBOY%2F0zz2dc1zZWRujsw%2FisxzIvPwPo55eGSeHZmfirEfA2TkaEZel5GNjPyZIQTmP9M89nXNc3V1by%2BNzJe%2F%2FN%2Bbz3qQ4Xlm5Jl1j18AAAAAAACAgcpJmpj7rozdfe%2B8MzLvaAbe74%2FMJc2w%2BuTIPCky55WC7HnN905ubrOkuc91zWPs7OJ8uyNzZXO%2Fd5TONbdZxbHf0dxmZXfH3Zlj8%2BNfl5Hvz8glGXl2Rp6ckSfl2AL0xd%2B85nsnN7dZ0tznuuYxdk7nYVw85%2FtDCM831j1uAQAAAAAAAAYuIy8fcLj6g%2FzuC54bbCKv%2Bq7n4rmM%2FMGAD3tl3eMWAAAAAAAAYOAy8rUZ%2BexAA9ZepnBXw61th31rwId8LiNfV%2Fe4BQAAAAAAABiKjPybgYas6494ovbgWI3V%2BiOeGPAh%2F67u8QoAAAAAAAAwNBl54UBD1ruP2lB7cKzG6u6jNgz4kO%2Bue7wCAAAAAAAADE1Gzs3IXQMLWb9yzMO1B8dqrL5yzMMDPNyujDy67vEKAAAAAAAAMFQZedXAgtYVJ91de3CsxuqGH7tngIe7qu5xCgAAAAAAADB0GfkfMvKRgQSt73zz6tqDYzVWS99064AOtTEjX1P3OAUAAAAAAACYERn50YGErYsvu7324FiN1evef9uADnVN3eMTAAAAAAAAYMZk5GtyrNN4emHrvL%2B8r%2FbgWI3VcV8eGcBhtmTk6XWPTwAAAAAAAIAZlZHXTDtwPWL9E7UHx2qs5qzbPIDDfLLucQkAAAAAAAAw4zJyXkbumXbouvHwJ2sPjw%2F1evqwbw7gMHsycl7d4xIAAAAAAACgFhl55bSD18vPsA563XXlz94%2BgMN8rO7xCAAAAAAAAFCbjPyxjLxlWsHroisE6HXX4g%2FcNs1D3JaR8%2BsejwAAAAAAAAC1ysi3Z%2BT3%2Bw5fj7rrG7kvvl97iHyo1g%2FiBzn39kemeZh31j0OAQAAAAAAAGaFjPyLaQWwD855rPYg%2BVCtjYc%2FOc1D3Fj3%2BAMAAAAAAACYNTLydRn5jb5D2HOXrK49SD5U63d%2BdfU0dn88I3%2Bh7vEHAAAAAAAAMKtk5GV9B7Ev%2FtpTmbG39jD50Kt%2Fz5fc%2BPg0DnFF3eMOAAAAAAAAYNbJyJdl5Jf7DmP%2F9IS1syBQPrTqy8eNTGP3v83IH6173AEAAAAAAADMSjk2lfu9fQWyZ7%2F71toD5UOt%2FmPj1j53fTAjz657vAEAAAAAAADMahn5pr5C2cO%2B%2BS%2B5J%2FbUHiofKrUn9uRh3%2FyXPndfUvc4AwAAAAAAADggZORH%2Bwpmz%2FvV1bUHy4dKXXhOv93nn6h7fAEAAAAAAAAcMLLf9dBfctsjuenwp2oPlw%2F22vrCHXn01x7uY1frngMAAAAAAAD0KiNPyMjtPYe0F71xde0B88FeV5x5Sx%2B77crIk%2BoeVwAAAAAAAAAHpIxcmJG7ewpqD3v6m7kv9tUeMh%2BstS%2F25RGPbOlxt90ZeVbd4wkAAAAAAADggJaR78jIJ3oKbM9dsrr2oPlgrf%2F65l7XPv9mRp5f9zgCAAAAAAAAOChk5IUZ%2Be2uQ9sj1j%2BRNx7zQO1h88FWt7z04Zzz4GM97PK9jLy07vEDAAAAAAAAcFDJyPdn5Pe7Dm8XfuSO2gPng60WX3Z7j7tcWfe4AQAAAAAAADgoZeTHegpwv3TcvbWHzgdL%2FdXL7%2Btxl0%2FVPV4AAAAAAAAADloZeWRGfjQj93UV4r7kn9bl%2FzzmwdrD5wO9%2Fukl6%2FKY%2F%2FlgD7t8IiOPrnu8AAAAAAAAABz0MvK%2FZbdrop947d35z0dsrj2EPlBr8wufzp%2F49F1dbv69jLwyI19Y9xgBAAAAAAAAOGRk5IUZubWrYHfxZbfnd1%2FwXO1h9IFW%2B2Jfnv3uW7vc%2FFsZeWnd4wIAAAAAAADgkJSR78jIf%2B4q4H3rb66uPZA%2B0Oqdv7q6y00fy8ildY8HAAAAAAAAgENaRi7MyO1Th7zfzfzMq2%2BuPZQ%2BUOqPf%2BIfMr7VzVrzuzLyrLrHAQAAAAAAAAARkZGvy8gvTxn2HrFuc%2F7xj6%2BtPZye7fWX8%2B7LH1q7oYtNV2XkL9X9%2FAEAAAAAAAAoyciXZeRHmx3RnUPfY258IG99yT%2FXHlLP1hr5oY153JdHpthsb0b%2BQUb%2BeN3PHQAAAAAAAIAOMvJNGblz0gD4h296MJ8%2B7Ju1h9Wzrb77gufyh296cIrNdmXkkrqfMwAAAAAAAABdyG6mdH%2FZ343mDT92T%2B2h9Wypv5x3Xxed56ZsBwAAAAAAADjQZOThGXlZRu7uGAgf9sjz%2BZlXf6328Lru%2BvNX3ZIvHP3uJJvsyciPZeThdT9XAAAAAAAAAPqUY93o12bk9yvD4Rc8%2B9387d9Ync%2FFc7UH2TNd%2Fxb%2Flv%2F1zbfmYdsnm%2FL%2Bixn5xrqfIwAAAAAAAAADkpFvz8hbOgbFiz9wWz5yxJbaQ%2B2Zqi0v3Ja%2F%2BK5bJ9nkzoxclpEvqvvZAQAAAAAAADBgGXlkRl6RY1OSTwyNf%2FiLj%2BfGH1pbe7g97Np6xAM57wvrO3y8LyOvyci5dT8vAAAAAAAAAIYsI%2Bdl5Kcqg%2FTDnv5mfum4e2sPuYdVN%2F3wg3n4xicrPtqTkZ%2FPyJPrfj4AAAAAAAAAzLCMfE1GfjQjHxkfJv%2FgB7nwI3fkjcc8UHvgPai67SWP5M%2B%2F97aM555r%2B%2BjRjPx4Rr627ucBAAAAAAAAQM0y8uiMvCojd40Pl%2FfsyXOXrM59sa%2F2ALzf2hf78gOLb8sXfPvZto92Nb%2Fz0XXffwAAAAAAAABmmYycm5HnZeQt48LmF932TF70xtX5vRc8UXsg3n1w%2Fq388On%2FlEfd%2BHTbR7c0v6M1zgEAAAAAAACYWka%2BNiMvz8h%2FbIXPL7ntkXznr67OR47YUntA3qm2vnBHvucNt%2BXLvvJA6e1%2FbH4X07QDAAAAAAAA0L%2BMXJiRn8rI9RmZedg3%2FyUXf%2BC2vOWlD9cemBd111HfyF9edmu%2BcMu25lvrm9e8sO77BwAAAAAAAMBBKCPnZeQ7MvKGjNycRzyyJc9dsjofnPPYjIfmD855LM9dsjp%2FaGRjRm5uXtM7MnJe3fcJAAAAAAAAgENMRp6ckRdm5Ofz%2BK%2F9j%2Fyl992UVy1anRvmrBt4YL5hzrq8atHq%2FKX33ZTHf%2B1%2FZOTnm%2Bc%2Bue77AAAAAAAAAAATZOTcjDw9f%2Fr238ilH7g0P3XOx%2FOe467NHXO%2BmhmrM2NjZmzOjO2lgHx7872NmbE6d8z5at5z3LX5qXM%2Bnks%2FcGn%2B9O2%2FkZGnZ%2BTcur8fAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHCg%2B%2F8BkH4vSiLLuTcAAAAASUVORK5CYII%3D&components%5B16%5D%5Bkey%5D=webgl&components%5B16%5D%5Bvalue%5D=data%3Aimage%2Fpng%3Bbase64%2CiVBORw0KGgoAAAANSUhEUgAAASwAAACWCAYAAABkW7XSAAARLUlEQVR4nO3c30sj%2Bp%2Ff8eef0YsWvheHHjgLww7swNC5mCBUKnghFeoiK1TwQtauUFlhBSEUwQWLraUuXgguXgiWWgQLFilDBrwQpqzz4%2Bg6%2FmhCJiTfTDJpQiZNyCSTZy%2F6XXYXvt%2Bz58fM%2BGPeD3jdB4Unft4QIYQQQgghhBBCCCGEEEIIIYQQQgghhBBCCCGEEEIIIYQQQgghhBBCCCGEEEIIIYQQQgghhBBCCCGEEEIIX4ySUJLX%2FTlCCOEf9JtgqSSu%2B7OEEMIP6knqN8Hyuj9LCCH8oL8brHgahhButN7fxiqiFUK4uTqS%2BC3BimiFEG6erqR%2BR7DiCB9CuFn%2BgWDFET6EcHN0xR8KlpK67s8YQgh0JPEjghX3rBDC9etI6kcGK6IVQrhePzFYcYQPIVyfjvgTgxVH%2BBDCl9eSxM8JVhzhQwhfXFuSPzNYcc8KIXxZHyT1C4IV0QohfDlt8RcGK6IVQvj8WpL4RMGKI3wI4fNqSfJTBSuO8CGEz6olqU8YrHgahhA%2Bn5b4iYMV0QohfHrvJfGZghXRCiF8Wi1JfsZgxRE%2BhPDpNCX1OYMVR%2FgQwifTFD9zsOJpGEL45d5L4gsFK6IVQvhl%2Fq8kv2CwIlohhJ%2BvIakvHCyN%2F6EVQvg5GuI1BCuO8CGEn6YqiWsKVjwNQwg%2FzTUHK6IVQvjx6pK65mBFtEIIP05dvAHBiiN8COGHVSVxg4IVX98JIfxuNy1Y8TQMIfxONUndsGBFtEIIv11NvIHBimiFEP6%2BqiRucLA0jvAhhL9xC4IVR%2FgQwv9XldRND5bx9Z0QAtyaYMU9K4SvXVkSVfGWBCuiFcLX7BYGS%2BMIH8LX6f9I6hYGK47wIXyNbmuwjCN8CF%2BXsiQq4i0NlsY9K4Svxx0IVkQrhK9FWVJ3IFgRrRC%2BBncoWHGED%2BEu%2B7UkyuJdCZZxhA%2Fh7rqDwYqnYQh3VUlSdzBYEa0Q7qI7HKyIVgh3TUm8w8GKI3wId8WvJXHXg2Uc4UO4G4o9UqWPWP6IlY9Y%2FYi1j1j%2FiI2P2PyIrY%2FY%2Foidj9j9iL2P6O1bPA1DuO2KH0iVPmD5A1Y%2BYPUD1j5g%2FQM2PmDzA7Y%2BYPsDdj5g9wP2PqC3cZ2IVgi3WrGJpSaWm1hpYrWJtSbWm9hoYrOJrSa2m9hpYreJvSZ6W9eKaIVwK%2BXekyi%2Bx9J7LL%2FHynusvsfae6y%2Fx8Z7bL7H1ntsv8fOe%2By%2Bx9579HYvcd0%2F%2BxDCT1SokSrWsFTDcg0rNazWsFbDeg0bNWzWsFXDdg07NezWsFdDb%2FfiCB%2FCbVN4R6r4DkvvsPwOK%2B%2Bw%2Bg5r77D%2BDhvvsPkOW%2B%2Bw%2FQ4777D7Dnvv0Nu%2BSjwNQ7hVCm%2Bx%2BBZLb7H8FitvsfoWa2%2Bx%2FhYbb7H5Fltvsf0WO2%2Bx%2BxZ7b9G7sYhWCLdBLkeikMdiHkt5LOexksdqHmt5rOexkcdmHlt5bOexk8duHnt59O4sohXCTVd4Q6rwBotvsPQGy2%2Bw8garb7D2ButvsPEGm2%2Bw9Qbbb7DzBrtvsPcGvUvLxBE%2BhButkCFVyGAxg6UMljNYyWA1g7UM1jPYyGAzg60MtjPYyWA3g70Mesd23b%2BPEMIPyF9i4RKLl1i6xPIlVi6xeom1S6xfYuMSm5fYusT2JXYusXuJvUv07i2ehiHcRLkzEvnXWHiNxddYeo3l11h5jdXXWHuN9dfYeI3N19h6je3X2HmN3dfYe43exZ1HtEK4cXKnJPMnWDjB4gmWTrB8gpUTrJ5g7QTrJ9g4weYJtk6wfYKdE%2ByeYO8Evas7jWiFcKPkXpHKv8TCSyy%2BxNJLLL%2FEykusvsTaS6y%2FxMZLbL7E1ktsv8TOS%2By%2BxN5L9C7veRzhQ7gxcseYP8bCMRaPsXSM5WOsHGP1GGvHWD%2FGxjE2j7F1jO1j7Bxj9xh7x%2Bgd33X%2FjkIIQOaIRO4Z5p9h4RkWn2HpGZafYeUZVp9h7RnWn2HjGTafYesZtp9h5xl2n2HvGXr3F1%2FfCeG6ZY9I5o4wf4SFIyweYekIy0dYOcLqEdaOsH6EjSNsHmHrCNtH2DnC7hH2jtCvY3HPCuE6vTkklTvE%2FCEWDrF4iKVDLB9i5RCrh1g7xPohNg6xeYitQ2wfYucQu4fYO0S%2FnkW0Qrgu2aeYe4r5p1h4isWnWHqK5adYeYrVp1h7ivWn2HiKzafYeortp9h5it2n2HuKfk1LxRE%2BhC8u8z9JZJ9g7gnmn2DhCRafYOkJlp9g5QlWn2DtCdafYOMJNp9g6wm2n2DnCXafYO8J%2BnUt%2FsoK4UvLHpDMHmDuAPMHWDjA4gGWDrB8gJUDrB5g7QDrB9g4wOYBtg6wfYCdA%2BweYO8A%2FXoWsQrhOmT2SWX3MbeP%2BX0s7GNxH0v7WN7Hyj5W97G2j%2FV9bOxjcx9b%2B9jex84%2Bdvext4%2Fe9f2PCFUI1yqzh9k9zO1hfg8Le1jcw9IelvewsofVPaztYX0PG3vY3MPWHrb3sLOH3T3s7aF3df89QhXCtbvcJZHZxewu5nYxv4uFXSzuYmkXy7tY2cXqLtZ2sb6LjV1s7mJrF9u72NnF7i72dtG7twhVCDfF5X8lkdnB7A7mdjC%2Fg4UdLO5gaQfLO1jZweoO1nawvoONHWzuYGsH2zvY2cHuDvZ20Luy%2FxahCuHGSW%2BTymxjdhtz25jfxsI2FrextI3lbaxsY3Uba9tY38bGNja3sbWN7W3sbGN3G3vb6G3ff4lQhXBjpbcws4XZLcxtYX4LC1tY3MLSFpa3sLKF1S2sbWF9Cxtb2NzC1ha2t7Czhd0t7G2ht3cRqhBussu%2FJJHexMwmZjcxt4n5TSxsYnETS5tY3sTKJlY3sbaJ9U1sbGJzE1ub2N7EziZ2N7G3id6%2BJd2MWIVw412uk0hvYGYDsxuY28D8BhY2sLiBpQ0sb2BlA6sbWNvA%2BgY2NrC5ga0NbG9gZwO7G9jbQG%2FT%2FjJCFcKtcbVOKr2OmXXMrmNuHfPrWFjH4jqW1rG8jpV1rK5jbR3r69hYx%2BY6ttaxvY6ddeyuY28dvR2LUIVw21ytYXoNM2uYXcPcGubXsLCGxTUsrWF5DStrWF3D2hrW17Cxhs01bK1hew07a9hdw94aerMXoQrhNjr7zySuVjG9iplVzK5ibhXzq1hYxeIqllaxvIqVVayuYm0V66vYWMXmKrZWsb2KnVXsrmJvFb2J%2B4sIVQi32tl%2FJHG1gukVzKxgdgVzK5hfwcIKFlewtILlFaysYHUFaytYX8HGCjZXsLWC7RXsrGB3BXsr6E3af4pQhXAnXPwHUlfLmF7GzDJmlzG3jPllLCxjcRlLy1hexsoyVpextoz1ZWwsY3MZW8vYXsbOMnaXsbeM3oxFqEK4Sy6W8GoJ00uYWcLsEuaWML%2BEhSUsLmFpCctLWFnC6hLWlrC%2BhI0lbC5hawnbS9hZwu4S9pbQ69y%2Fj1CFcOecLZC4WMSrRUwvYmYRs4uYW8T8IhYWsbiIpUUsL2JlEauLWFvE%2BiI2FrG5iK1FbC9iZxG7i9hbRK9jf07SP49YhXAnnS2QuFjAqwVML2BmAbMLmFvA%2FAIWFrC4gKUFLC9gZQGrC1hbwPoCNhawuYCtBWwvYGcBuwvYW0C%2F%2FCJUIdxlF0lSF0m8SmI6iZkkZpOYS2I%2BiYUkFpNYSmI5iZUkVpNYS2I9iY0kNpPYSmI7iZ0kdpPYS6Jfav8uQhXCV%2BFintTFPF7NY3oeM%2FOYncfcPObnsTCPxXkszWN5HivzWJ3H2jzW57Exj815bM1jex4789idx948%2BvkXoQrha3H2ZyTO5%2FBiDq%2FmMD2HmTnMzmFuDvNzWJjD4hyW5rA8h5U5rM5hbQ7rc9iYw%2BYctuawPYedOezOYW8O%2FVz7swhVCF%2Bdsz8lcT6LF7N4NYvpWczMYnYWc7OYn8XCLBZnsTSL5VmszGJ1FmuzWJ%2FFxiw2Z7E1i%2B1Z7MxidxZ7s%2Bin3p9GqEL4ap39W1LnM3gxg1czmJ7BzAxmZzA3g%2FkZLMxgcQZLM1iewcoMVmewNoP1GWzMYHMGWzPYnsHODHZnsDeDfrpFqEL42p39CanzabyYxqtpTE9jZhqz05ibxvw0FqaxOI2laSxPY2Uaq9NYm8b6NDamsTmNrWlsT2NnGrvT2JtGf%2FkiVCEE%2BP7fkDibwvMpvJjCqylMT2FmCrNTmJvC%2FBQWprA4haUpLE9hZQqrU1ibwvoUNqawOYWtKWxPYWcKu1PYm0J%2F7v6YpH8csQoh%2FMb3kyTOJvF8Ei8m8WoS05OYmcTsJOYmMT%2BJhUksTmJpEsuTWJnE6iTWJrE%2BiY1JbE5iaxLbk9iZxO4k9ibRn7cIVQjh7%2FvrCVJnE3g%2BgRcTeDWB6QnMTGB2AnMTmJ%2FAwgQWJ7A0geUJrExgdQJrE1ifwMYENiewNYHtCexMYHcCexPoT1uEKoTw2%2F31OKmzcTwfx4txvBrH9DhmxjE7jrlxzI9jYRyL41gax%2FI4VsaxOo61cayPY2Mcm%2BPYGsf2OHbGsTuOvXH0x%2BxfR6hCCD%2Fg%2Bz8icTqGZ2N4PoYXY3g1hukxzIxhdgxzY5gfw8IYFsewNIblMayMYXUMa2NYH8PGGDbHsDWG7THsjGF3DHtj6A%2FtjyJUIYQf4fs%2FJHE6imejeD6KF6N4NYrpUcyMYnYUc6OYH8XCKBZHsTSK5VGsjGJ1FGujWB%2FFxig2R7E1iu1R7IxidxR7o%2BhvX4QqhPDjnYyQOh3BsxE8H8GLEbwawfQIZkYwO4K5EcyPYGEEiyNYGsHyCFZGsDqCtRGsj2BjBJsj2BrB9gh2RrA7gr0R9O%2FuX0WoQgg%2Fw8kwqdNhPBvG82G8GMarYUwPY2YYs8OYG8b8MBaGsTiMpWEsD2NlGKvDWBvG%2BjA2hrE5jK1hbA9jZxi7w9gbRofRfxmhCiH8AidDeDqEZ0N4PoQXQ3g1hOkhzAxhdghzQ5gfwsIQFoewNITlIawMYXUIa0NYH8LGEDaHsDWE7SHsDGF3CB0i6VDEKoTwCzwfJHEyiKeDeDaI54N4MYhXg5gexMwgZgcxN4j5QSwMYnEQS4NYHsTKIFYHsTaI9UFsDGJzEFuD2B7EziB%2BHIxQhRA%2BgVcDpE4G8HQAzwbwfAAvBvBqANMDmBnA7ADmBjA%2FgIUBLA5gaQDLA1gZwOoA1gawPoCNAWwOYGsA2%2F8iQhVC%2BIRe%2FXNSJ%2F142o9n%2FXjejxf9eNWP6X7M9GO2H3P9mO%2FHQj8W%2B7HUj%2BV%2BrPRjtR9r%2FVjvx0Y%2FtvojVCGEz%2BBVH5704WkfnvXheR9e9OFVH6b7MNOH2T7M9WG%2BDwt9WOzDUh%2BW%2B7DSh9U%2BrPXh%2B74IVQjhM3meIPHqMZ48xtPHePYYzx%2FjxWO8eozpx5h5jNnHmHuM%2BcdYeIzFx1h6jOXHWHmM1USEKoTwmb14ROrVIzx5hKeP8OwRnj%2FCi0d49QjTjzDzCLOPMPcI84%2Bw8AiLj7D0CMv%2FLEIVQvhCXjwk9eohnjzE04d49hDPH%2BLFQ7x6iOmHmHmI2YeYe4j5h1h4iG8fRqhCCF%2FYiwf46gGePMDTB3j2AM8f4MUDvHqA6QeYeYDZB5h7gPk%2FIPnrP4hYhRC%2BsOe%2FT%2BLFfXx1H0%2Fu4%2Bl9PLuP5%2Ffx4j5e3cf0fczcx%2Bx9zP1%2BhCqEcE2e3yP54h6%2Buocn9%2FD0Hp7dw%2FN7eHEPr%2B5h%2Bh5m70WoQgjX7PnvkXrxHb76Dk%2B%2Bw9Pv8Ow7PP8OL77D%2F%2F1dhCqEcEM8%2FxZffIuvvsWTb%2FH0Wzz7Fs%2F%2FaYQqhHCD%2FK9vSDz%2FBl98g6%2B%2BwZNv8PSbCFUI4Qb6q1%2BRfP4rfPEr%2FP5XEaoQwg12%2FE9IPf%2FHEaoQwi3wV%2F8oYhVCCCGEEEIIIYQQQgghhBBCCCGEEMJn9v8Arf4GlZAexhsAAAAASUVORK5CYII%3D~extensions%3AANGLE_instanced_arrays%3BEXT_blend_minmax%3BEXT_color_buffer_half_float%3BEXT_frag_depth%3BEXT_shader_texture_lod%3BEXT_texture_filter_anisotropic%3BEXT_disjoint_timer_query%3BOES_element_index_uint%3BOES_standard_derivatives%3BOES_texture_float%3BOES_texture_float_linear%3BOES_texture_half_float%3BOES_texture_half_float_linear%3BOES_vertex_array_object%3BWEBGL_color_buffer_float%3BWEBGL_compressed_texture_etc1%3BWEBGL_compressed_texture_s3tc%3BWEBGL_depth_texture%3BWEBGL_draw_buffers%3BWEBGL_lose_context%3BMOZ_WEBGL_lose_context%3BMOZ_WEBGL_compressed_texture_s3tc%3BMOZ_WEBGL_depth_texture~webgl+aliased+line+width+range%3A%5B1%2C+1%5D~webgl+aliased+point+size+range%3A%5B1%2C+1024%5D~webgl+alpha+bits%3A8~webgl+antialiasing%3Ayes~webgl+blue+bits%3A8~webgl+depth+bits%3A16~webgl+green+bits%3A8~webgl+max+anisotropy%3A16~webgl+max+combined+texture+image+units%3A32~webgl+max+cube+map+texture+size%3A16384~webgl+max+fragment+uniform+vectors%3A1024~webgl+max+render+buffer+size%3A16384~webgl+max+texture+image+units%3A16~webgl+max+texture+size%3A16384~webgl+max+varying+vectors%3A30~webgl+max+vertex+attribs%3A16~webgl+max+vertex+texture+image+units%3A16~webgl+max+vertex+uniform+vectors%3A4096~webgl+max+viewport+dims%3A%5B32767%2C+32767%5D~webgl+red+bits%3A8~webgl+renderer%3AMozilla~webgl+shading+language+version%3AWebGL+GLSL+ES+1.0~webgl+stencil+bits%3A0~webgl+vendor%3AMozilla~webgl+version%3AWebGL+1.0~webgl+vertex+shader+high+float+precision%3A23~webgl+vertex+shader+high+float+precision+rangeMin%3A127~webgl+vertex+shader+high+float+precision+rangeMax%3A127~webgl+vertex+shader+medium+float+precision%3A23~webgl+vertex+shader+medium+float+precision+rangeMin%3A127~webgl+vertex+shader+medium+float+precision+rangeMax%3A127~webgl+vertex+shader+low+float+precision%3A23~webgl+vertex+shader+low+float+precision+rangeMin%3A127~webgl+vertex+shader+low+float+precision+rangeMax%3A127~webgl+fragment+shader+high+float+precision%3A23~webgl+fragment+shader+high+float+precision+rangeMin%3A127~webgl+fragment+shader+high+float+precision+rangeMax%3A127~webgl+fragment+shader+medium+float+precision%3A23~webgl+fragment+shader+medium+float+precision+rangeMin%3A127~webgl+fragment+shader+medium+float+precision+rangeMax%3A127~webgl+fragment+shader+low+float+precision%3A23~webgl+fragment+shader+low+float+precision+rangeMin%3A127~webgl+fragment+shader+low+float+precision+rangeMax%3A127~webgl+vertex+shader+high+int+precision%3A0~webgl+vertex+shader+high+int+precision+rangeMin%3A31~webgl+vertex+shader+high+int+precision+rangeMax%3A30~webgl+vertex+shader+medium+int+precision%3A0~webgl+vertex+shader+medium+int+precision+rangeMin%3A31~webgl+vertex+shader+medium+int+precision+rangeMax%3A30~webgl+vertex+shader+low+int+precision%3A0~webgl+vertex+shader+low+int+precision+rangeMin%3A31~webgl+vertex+shader+low+int+precision+rangeMax%3A30~webgl+fragment+shader+high+int+precision%3A0~webgl+fragment+shader+high+int+precision+rangeMin%3A31~webgl+fragment+shader+high+int+precision+rangeMax%3A30~webgl+fragment+shader+medium+int+precision%3A0~webgl+fragment+shader+medium+int+precision+rangeMin%3A31~webgl+fragment+shader+medium+int+precision+rangeMax%3A30~webgl+fragment+shader+low+int+precision%3A0~webgl+fragment+shader+low+int+precision+rangeMin%3A31~webgl+fragment+shader+low+int+precision+rangeMax%3A30&components%5B17%5D%5Bkey%5D=adblock&components%5B17%5D%5Bvalue%5D=false&components%5B18%5D%5Bkey%5D=has_lied_languages&components%5B18%5D%5Bvalue%5D=false&components%5B19%5D%5Bkey%5D=has_lied_resolution&components%5B19%5D%5Bvalue%5D=false&components%5B20%5D%5Bkey%5D=has_lied_os&components%5B20%5D%5Bvalue%5D=false&components%5B21%5D%5Bkey%5D=has_lied_browser&components%5B21%5D%5Bvalue%5D=false&components%5B22%5D%5Bkey%5D=touch_support&components%5B22%5D%5Bvalue%5D%5B%5D=0&components%5B22%5D%5Bvalue%5D%5B%5D=false&components%5B22%5D%5Bvalue%5D%5B%5D=false&components%5B23%5D%5Bkey%5D=js_fonts&components%5B23%5D%5Bvalue%5D%5B%5D=Arial&components%5B23%5D%5Bvalue%5D%5B%5D=Arial+Unicode+MS&components%5B23%5D%5Bvalue%5D%5B%5D=Book+Antiqua&components%5B23%5D%5Bvalue%5D%5B%5D=Bookman+Old+Style&components%5B23%5D%5Bvalue%5D%5B%5D=Calibri&components%5B23%5D%5Bvalue%5D%5B%5D=Cambria&components%5B23%5D%5Bvalue%5D%5B%5D=Cambria+Math&components%5B23%5D%5Bvalue%5D%5B%5D=Century&components%5B23%5D%5Bvalue%5D%5B%5D=Century+Gothic&components%5B23%5D%5Bvalue%5D%5B%5D=Century+Schoolbook&components%5B23%5D%5Bvalue%5D%5B%5D=Comic+Sans+MS&components%5B23%5D%5Bvalue%5D%5B%5D=Consolas&components%5B23%5D%5Bvalue%5D%5B%5D=Courier&components%5B23%5D%5Bvalue%5D%5B%5D=Courier+New&components%5B23%5D%5Bvalue%5D%5B%5D=Garamond&components%5B23%5D%5Bvalue%5D%5B%5D=Georgia&components%5B23%5D%5Bvalue%5D%5B%5D=Helvetica&components%5B23%5D%5Bvalue%5D%5B%5D=Impact&components%5B23%5D%5Bvalue%5D%5B%5D=Lucida+Bright&components%5B23%5D%5Bvalue%5D%5B%5D=Lucida+Calligraphy&components%5B23%5D%5Bvalue%5D%5B%5D=Lucida+Console&components%5B23%5D%5Bvalue%5D%5B%5D=Lucida+Fax&components%5B23%5D%5Bvalue%5D%5B%5D=Lucida+Handwriting&components%5B23%5D%5Bvalue%5D%5B%5D=Lucida+Sans&components%5B23%5D%5Bvalue%5D%5B%5D=Lucida+Sans+Typewriter&components%5B23%5D%5Bvalue%5D%5B%5D=Lucida+Sans+Unicode&components%5B23%5D%5Bvalue%5D%5B%5D=Microsoft+Sans+Serif&components%5B23%5D%5Bvalue%5D%5B%5D=Monotype+Corsiva&components%5B23%5D%5Bvalue%5D%5B%5D=MS+Gothic&components%5B23%5D%5Bvalue%5D%5B%5D=MS+Outlook&components%5B23%5D%5Bvalue%5D%5B%5D=MS+PGothic&components%5B23%5D%5Bvalue%5D%5B%5D=MS+Reference+Sans+Serif&components%5B23%5D%5Bvalue%5D%5B%5D=MS+Sans+Serif&components%5B23%5D%5Bvalue%5D%5B%5D=MS+Serif&components%5B23%5D%5Bvalue%5D%5B%5D=Palatino+Linotype&components%5B23%5D%5Bvalue%5D%5B%5D=Segoe+Print&components%5B23%5D%5Bvalue%5D%5B%5D=Segoe+Script&components%5B23%5D%5Bvalue%5D%5B%5D=Segoe+UI&components%5B23%5D%5Bvalue%5D%5B%5D=Segoe+UI+Symbol&components%5B23%5D%5Bvalue%5D%5B%5D=Tahoma&components%5B23%5D%5Bvalue%5D%5B%5D=Times&components%5B23%5D%5Bvalue%5D%5B%5D=Times+New+Roman&components%5B23%5D%5Bvalue%5D%5B%5D=Trebuchet+MS&components%5B23%5D%5Bvalue%5D%5B%5D=Verdana&components%5B23%5D%5Bvalue%5D%5B%5D=Wingdings&components%5B23%5D%5Bvalue%5D%5B%5D=Wingdings+2&components%5B23%5D%5Bvalue%5D%5B%5D=Wingdings+3';
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