﻿// 2020.06.27
////////////////////////  Создание  списка  видео   ///////////////////////////
#define mpiJsonInfo 40032 // Идентификатор для хранения json информации о фильме
#define mpiKPID     40033 // Идентификатор для хранения ID кинопоиска

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
int       gnTotalItems  = 0; 
TDateTime gStart        = Now;
string    gsAPIUrl      = "http://api.lostcut.net/hdkinoteatr/";
int       gnDefaultTime = 6000;

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
// Создание ссылки на видео
THmsScriptMediaItem CreateMediaItem(THmsScriptMediaItem Folder, string sTitle, string sLink, string sImg, int nTime, string sGrp='') {
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, Folder.ItemID, sGrp);
  Item[mpiTitle     ] = sTitle;
  Item[mpiThumbnail ] = sImg;
  Item[mpiCreateDate] = IncTime(gStart,0,-gnTotalItems,0,0); gnTotalItems++;
  Item[mpiTimeLength] = HmsTimeFormat(nTime)+'.000';
  if (HmsRegExMatch('(?:/embed/|v=)([\\w-_]+)', sLink, sImg))
    Item[mpiThumbnail] = 'http://img.youtube.com/vi/'+sImg+'/1.jpg';
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
// Создание информационной ссылки
void CreateInfoItem(string sKey, string sVal) {
  THmsScriptMediaItem Item; sVal = Trim(sVal);
  if (sVal=="") return;
  CreateMediaItem(FolderItem, sKey+': '+sVal, 'Info'+sVal, 'http://wonky.lostcut.net/vids/info.jpg', 7);
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
// Конвертирование строки даты вида ГГГГ-ММ-ДД в дату
TDateTime ConvertToDate(string sData) {
  string y, m, d, h, i, s;
  HmsRegExMatch3('(\\d+)-(\\d+)-(\\d+)', sData, y, m, d);
  HmsRegExMatch3('(\\d+):(\\d+):(\\d+)', sData, h, i, s);
  return StrToDateDef(d+'.'+m+'.'+y, Now);
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
// Расшифровка текста плеера playerjs-alloha-new с allohastream.com
string AllohaDecode(string sData, string &sHtml) {
  string pre, salt, iv, ct, key, js; int i;
  Variant trash = ["##P3w7Xl58Kj4qPj8/Pl58Xjx8Pnw/ISrihJYofDshP17ihJY+", "##Pzs+KSEoKjt8fD58KjxefCp8XipgPj98KHwqPnx8fl1bfD58Kl4q", "##PGBeKmAqPnzihJYqKuKEll0/Wyo7fHw+fCrihJY7Xipg4oSWKj4=", "##fFs+KuKElj5eP1s7fHw+fCo8KirihJZdfHxePCoqfA==", "##OyE/XuKElj4qXipgfHxePCrihJZ8fF4qYF4qKnzihJYqfl1bfD58"];
  for (i=0; i < Length(trash) ; i++) sData = ReplaceStr(sData, trash[i], "");
  for (i=0; i < Length(trash) ; i++) sData = ReplaceStr(sData, trash[i], ""); // Иногда мусор встраивается в мусор, поэтому проходим два раза
    pre = LeftCopy(sData, 2);
  if (pre=="#9") {
    return hd0_decode(Copy(sData, 3, Length(sData)));
  }
  if (pre=="#7") {
    js = sHtml;
    TRegExpr Regex = TRegExpr.Create('["\'](\\w+)["\'],(\\d+),["\'](\\w+)["\'],(\\d+),(\\d+),');
    for (i=0; i<9; i++) if (Regex.Search(js)) js = JsUnpack(Regex.Match(1), StrToInt(Regex.Match(2)), Regex.Match(3), StrToInt(Regex.Match(4)), StrToInt(Regex.Match(5)));
    Regex.Free();
    if (!HmsRegExMatch('.*=\\s*["\'](.*?)["\']', js, key)) HmsLogMessage(2, 'Не удалось найти значение Key для расшифровки alloha!');
    salt = Copy(sData, Length(sData)-15, 16);
    iv   = Copy(sData, Length(sData)-49, 32);
    ct   = Copy(sData, 3, Length(sData)-54);
    return CryptoJsAesDecrypt(key, ct, iv, salt);
  }
  if (pre=="#0") {
    return HmsBase64Decode(Copy(sData, 3, Length(sData)));    
  }
  return sData;
}

double pow(double base, int expon) {
  if (expon==0) return 1;
  if (base ==0) return 0;
  if ((base>=0) && (expon<0)) return 1 / Exp(Abs(expon) * Ln(base));
  return Exp(expon * Ln(base));
}

////////////////////////////////////////////////////////////////////////////
string BaseConverter(string d, int e, int f) {
  string g = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+/';
  string h = Copy(g, 1, e);
  string i = Copy(g, 1, f);
  string s = ""; for (int n=Length(d); n>0 ; n--) s += d[n]; d = s; // reverse
  double j = 0;
  for(int c= 0; c < Length(d); c++) {
    int p = pos(d[int(c+1)], h);
    if (p > 0) j += Round((p-1) * pow(e, c));
  }
  string k = '';
  while (j > 0) {
    k = i[Round(j % f)+1] + k;
    j = Round((j - (j % f)) / f);
  }
  if (k=='') return '0'; else return k;
}

///////////////////////////////////////////////////////////////////////////////
// Распаковка js-скрипта
string JsUnpack(string h, int u, string n, int t, int e, string r="") {
  r = "";
  for (int i=0; i < Length(h); i++) {
    string s = ""; while (h[i+1] != n[e+1]) { s += h[i+1]; i++; }
    for (int j=0; j < Length(n); j++) s = ReplaceStr(s, n[j+1], str(j));
    r += chr(StrToInt(BaseConverter(s, e, 10)) - t);
  }
  return r;
}

///////////////////////////////////////////////////////////////////////////////
// Раскодирование ссылки с сайта bazon.info
string BazonDecode(string data, string path) {
  int i = StrToInt(Copy(data, Length(data), 1));
  data = Copy(data, 1, Length(data)-1);
  if (ProgramVersion >= "3.0") {
    string s = 'decodeBase64=function(f){var g={},b=65,d=0,a,c=0,h,e="",k=String.fromCharCode,l=f.length;for(a="";91>b;)a+=k(b++);a+=a.toLowerCase()+"0123456789+/";for(b=0;64>b;b++)g[a.charAt(b)]=b;for(a=0;a<l;a++)for(b=g[f.charAt(a)],d=(d<<6)+b,c+=6;8<=c;)((h=d>>>(c-=8)&255)||a<l-2)&&(e+=k(h));return e};';
    data = jsEval(s+";(decodeBase64('"+data+"'))");
  } else {
    data = HmsBase64Decode(data);
  }
  string c = Copy(data, 1, i)+"TZ#x!z40";
  string d = Copy(data, i+1, Length(data)-i);
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
// Создание ссылок на фильм или сериал
void CreateLinks() {
  string sHtml, sData, sName, sLink, sVal, sTrans, sTransID, sQual, sAdd, sSeasonName, sGrp, sAPI, sImg, sHeaders, sServ, js, path, hash, id;
  THmsScriptMediaItem Item; int i, n, nTime; TJsonObject VIDEO, PLAYLIST, VLINKS, PL, SEASON, TRANSLATION; TJsonArray SEASONS, EPISODES;
  VIDEO    = TJsonObject.Create(); TRegExpr RE;
  PLAYLIST = TJsonObject.Create();
  VLINKS   = TJsonObject.Create();
  PL       = TJsonObject.Create();
  if ((Trim(FolderItem[mpiJsonInfo])=="") && HmsRegExMatch('.*/(\\d+)-', mpFilePath, id)) {
    FolderItem[mpiJsonInfo] = HmsUtf8Decode(HmsDownloadURL(gsAPIUrl+"videos?id="+sVal));
    HmsRegExMatch("^\\[(.*)\\]$", FolderItem[mpiJsonInfo], FolderItem[mpiJsonInfo]);
  }
  try {
    VIDEO.LoadFromString(FolderItem[mpiJsonInfo]);
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
            if (Pos('bazon.site', sLink)>0)
            sHtml = HmsDownloadURL(sLink, 'Referer: http://4h0y.gitlab.io/\r\nOrigin: '+sServ);
            if (HmsRegExMatch('<script>eval(\\(.*?\\))</script>', sHtml, js, 1, PCRE_SINGLELINE)) sData = jsEval(js);
            HmsRegExMatch('path:"(.*?)"', sData, path);
            if (HmsRegExMatch("eval(\\(.*?split\\('\\|'\\),0,{}\\)\\))", sData, js, 1, PCRE_SINGLELINE)) sData = jsEval(js);
            HmsRegExMatch('file="(.*?)";', sData, sVal);
            sVal = BazonDecode(ReplaceStr(sVal, '"+"', ""), path);
            mpFilePath = sLink;
            CreateSeriesFromPlaylist(FolderItem, sVal, 'bazon');
          } else {
            sName = Trim('[bazon] '+VLINKS.S['bazon\\translate']+' '+VLINKS.S['bazon\\quality']);
            Item = CreateMediaItem(FolderItem, sName, sLink, mpThumbnail, gnDefaultTime);
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
                  Item = CreateMediaItem(FolderItem, Format('%.2d cерия', [StrToInt(mpSeriesEpisodeNo)]), sLink+id, mpThumbnail, gnDefaultTime, sGrp);
                  Item.ItemParent[mpiFolderSortOrder] = 'mpTitle';
                  Item.ItemParent.Sort('+mpTitle');
                }
              }
            }
          } else {
            sName = Trim('[ustore] '+VLINKS.S['ustore\\translate']+' '+VLINKS.S['ustore\\quality']);
            Item = CreateMediaItem(FolderItem, sName, sLink, mpThumbnail, gnDefaultTime);
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
                  sData  = AllohaDecode(sVal, sHtml);
                  HmsRegExMatch('"file":"(.*?)"', sData, sVal);
                  sLink  = AllohaDecode(sVal, sHtml);
                  sGrp = Trim('alloha Сезон '+PLAYLIST.Names[i]+' '+ReplaceStr(TRANSLATION.S['translation'], 'Не требуется', ''));
                  Item = CreateMediaItem(FolderItem, Format('%.2d cерия', [StrToInt(mpSeriesEpisodeNo)]), sLink, mpThumbnail, gnDefaultTime, sGrp);
                  Item.ItemParent[mpiFolderSortOrder] = 'mpTitle';
                  Item.ItemParent.Sort('+mpTitle');
                }
              }
            }
          } else {
            sName = Trim('[alloha] '+VLINKS.S['alloha\\translate']+' '+VLINKS.S['alloha\\quality']);
            Item = CreateMediaItem(FolderItem, sName, sLink, mpThumbnail, gnDefaultTime);
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
                    Item = CreateMediaItem(FolderItem, sName, sLink, mpThumbnail, gnDefaultTime, sGrp);
                  } while (RE.SearchAgain());
                } //for (n=0; n<EPISODES.Length; n++)
              } //for (i=0; i<SEASONS.Length; i++)
            } //for (int nTrans=0; nTrans<PLAYLIST.Count; nTrans++)
            RE.free; TRANS.Free;
          } else {
            Item = CreateMediaItem(FolderItem, sName, sLink, mpThumbnail, gnDefaultTime);
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
                  Item = CreateMediaItem(FolderItem, sName, sLink, sImg, nTime, sGrp);
                }
              }
            }
          } else {
            Item = CreateMediaItem(FolderItem, '[collaps] '+VIDEO.S['name'], VLINKS.S['collaps\\iframe'], mpThumbnail, gnDefaultTime);
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
                Item = CreateMediaItem(FolderItem, sName, sLink, mpThumbnail, gnDefaultTime, sGrp);
              } while (RESeries.SearchAgain());
              Item = FolderItem.AddFolder(sGrp);
              if (Item!=nil) Item.Sort('+mpTitle');
            } while (RE.SearchAgain());
          } else {
            Item = CreateMediaItem(FolderItem, '[iframe] '+VIDEO.S['name'], VLINKS.S['iframe\\iframe'], mpThumbnail, gnDefaultTime);
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
                  Item = CreateMediaItem(FolderItem, sName, sLink, mpThumbnail, gnDefaultTime, sGrp);
                } while (RESeries.SearchAgain());
              } while (RE.SearchAgain());
            } finally { RE.Free; RESeries.Free; }
          } else {
            sName = '[hdvb] '+VLINKS.S['hdvb\\translate']+' '+VLINKS.S['hdvb\\quality'];
            Item  = CreateMediaItem(FolderItem, sName, VLINKS.S['hdvb\\iframe'], mpThumbnail, gnDefaultTime);
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
                  CreateMediaItem(FolderItem, sName+' серия', "http:"+SERIES.Values[sName], mpThumbnail, nTime, sGrp);
                }
              }
              
            } else {
              // Это фильм - создаём одну ссылку на видео
              Item = CreateMediaItem(FolderItem, '[kodik] '+PLAYLIST.S['title'], VLINKS.S['kodik\\iframe'], mpThumbnail, nTime);
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
              Item = CreateMediaItem(FolderItem, sName, sServ+sLink, mpThumbnail, gnDefaultTime, sGrp);
            } while (RE.SearchAgain());
            RE.Free;
          } else {
            sName = Trim('[ONIK] '+VLINKS.S['ONIK\\translate']+' '+VLINKS.S['ONIK\\quality']);
            Item  = CreateMediaItem(FolderItem, sName, sOriginalLink, mpThumbnail, gnDefaultTime);
            FillVideoInfo(Item);
          }
          
          
        } // if (VLINKS.B['ONIK'])
      } except { }
      // --------------------------------------------------------------------
      if (VLINKS.B['trailer']) {
        sLink = VLINKS.S['trailer\\iframe'];
        Item = CreateMediaItem(FolderItem, 'Трейлер', sLink, mpThumbnail, gnDefaultTime);
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
        if (bExist) { sName = "Удалить из избранного"; sLink = "-FavDel="+FolderItem.ItemParent.ItemID+";"+mpFilePath; }
        else        { sName = "Добавить в избранное" ; sLink = "-FavAdd="+FolderItem.ItemParent.ItemID+";"+mpFilePath; }
        CreateMediaItem(FolderItem, sName, sLink, 'http://wonky.lostcut.net/icons/ok.png', 1, sName);
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
// Создание списка видео
void CreateVideosList(string sParams='') {
  string sData, sLink, sName, sImg, sKPID, sYear, sVal, sGrp="", sGroupKey="";
  int i, nTime, iCnt=0, nGrp=0, nSortCount=0, nMaxInGroup=100, nMinInGroup=30;
  TJsonObject JSON, VIDEO; TJsonArray JARRAY; THmsScriptMediaItem Item, Folder=FolderItem;
  
  // Параметры
  bool bYearInTitle = Pos('--yearintitle', mpPodcastParameters) > 0; // Добавлять год к названию
  bool bNoFolders   = Pos('--nofolders'  , mpPodcastParameters) > 0; // Режим "без папкок" для фильмов
  bool bNumetare    = Pos('--numerate'   , mpPodcastParameters) > 0; // Нумеровать созданные ссылки
  bool bAdd2NameKP  = Pos('--kpinname'   , mpPodcastParameters) > 0; // Добавлять рейтинг Кинопоиск к названию
  bool bAdd2NameIMDb= Pos('--imdbinname' , mpPodcastParameters) > 0; // Добавлять рейтинг IMDb к названию
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
    if (ProgramVersion >= '3.0')
      sParams += HmsHttpEncode(mpTitle);
    else
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
        HmsRegExMatch("(http[^\"']+)", HmsJsonDecode(VIDEO.S['link']), sLink);
        sLink = ReplaceStr(sLink, "moon.hdkinoteatr.com", "moonwalk.cc");
        if (bAdd2NameKP && VIDEO.B['rating_kp']) {
          sName += " КП: "+VIDEO.S['rating_kp'];
        }
        if (bAdd2NameIMDb && VIDEO.B['rating_imdb']) {
          sName += " IMDb: "+VIDEO.S['rating_imdb'];
        }
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
  if ((Trim(Podcast[550])=='') || (DateTimeToTimeStamp1970(Now, false)-StrToIntDef(Podcast[mpiTimestamp], 0) < 14400)) return; // раз в 4 часа
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
      if      (sName=='CreatePodcastFeeds'  ) { mpiSHA=100701; mpiScript=571; sMsg='Требуется запуск "Создать ленты подкастов"'; } // Это сприпт создания покаст-лент   (Alt+1)
      else if (sName=='CreateFolderItems'   ) { mpiSHA=100702; mpiScript=530; sMsg='Требуется обновить раздел заново';           } // Это скрипт чтения списка ресурсов (Alt+2)
      else if (sName=='FolderItemProperties') { mpiSHA=100703; mpiScript=510; sMsg='Требуется обновить раздел заново';           } // Это скрипт чтения дополнительных в RSS (Alt+3)
      else if (sName=='MediaResourceLink'   ) { mpiSHA=100704; mpiScript=550; sMsg=''; }                                           // Это скрипт получения ссылки на ресурс  (Alt+4)
      else continue;                         // Если файл не определён - пропускаем
      if (Podcast[mpiSHA]!=JFILE.S['sha']) { // Проверяем, требуется ли обновлять скрипт?
        sData = HmsDownloadURL(JFILE.S['download_url'], "Accept-Encoding: gzip, deflate", true); // Загружаем скрипт
        sData = ReplaceStr(sData, 'п»ї', ''); // Remove BOM
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
{
  // Проверка на упоротых, которые пытаются запустить "Обновление подкастов" для всего подкаста разом
  if (InteractiveMode && (HmsCurrentMediaTreeItem.ItemClassName=='TVideoPodcastsFolderItem')) {
    HmsCurrentMediaTreeItem.DeleteChildItems(); // Дабы обновления всех подкастов не запустилось - удаляем их.
    ShowMessage('Обновление всех разделов разом запрещено!\nДля восстановления структуры\nзапустите "Создать ленты подкастов".');
    return;
  } 
  FolderItem.DeleteChildItems();

  if ((Pos('--nocheckupdates' , mpPodcastParameters)<1) && (mpComment=='--update')) CheckPodcastUpdate(); // Проверка обновлений подкаста
 
  if (LeftCopy(mpFilePath, 4)=='http') CreateLinks(); // создание ссылок на серии
  else                                 CreateVideosList(mpFilePath);
  
  HmsLogMessage(1, Podcast[mpiTitle]+' "'+mpTitle+'": Создано элементов - '+IntToStr(gnTotalItems));
}