// 2017.12.02
//////////////////  Получение ссылок на медиа-ресурс   ////////////////////////
#define mpiSeriesInfo 10323  // Идентификатор для хранения информации о сериях

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
string    gsUrlBase    = "http://filmix.cc";
bool      gbHttps      = (LeftCopy(gsUrlBase, 5)=='https');
int       gnTime       = 6000;
int       gnTotalItems = 0;
int       gnQual       = 0;  // Минимальное качество для отображения
TDateTime gStart       = Now;
string    gsSeriesInfo = ''; // Информация о сериях сериала (названия)
string    gsHeaders = mpFilePath+'\r\n'+
                  'Accept: application/json, text/javascript, */*; q=0.01\r\n'+
                  'Accept-Encoding: identity\r\n'+
                  'Origin: '+gsUrlBase+'\r\n'+
                  'User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36\r\n'+
                  'X-Requested-With: XMLHttpRequest\r\n';

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
// https://hms.lostcut.net/viewtopic.php?id=80
// ---- Расшифровка закодированного текста плеера uppod -----------------------
string DecodeUppodText (string sData) {
  char char1, char2; int i;
  variant Client_codec_a = ["l", "u", "T", "D", "Q", "H", "0", "3", "G", "1", "f", "M", "p", "U", "a", "I", "6", "k", "d", "s", "b", "W", "5", "e", "y", "="];
  variant Client_codec_b = ["w", "g", "i", "Z", "c", "R", "z", "v", "x", "n", "N", "2", "8", "J", "X", "t", "9", "V", "7", "4", "B", "m", "Y", "o", "L", "h"];
  
  sData = ReplaceStr(sData, "\n", "");
  for (i=0; i<Length(Client_codec_a); i++) {
    char1 = Client_codec_b[i];
    char2 = Client_codec_a[i];
    sData = ReplaceStr(sData, char1, "___");
    sData = ReplaceStr(sData, char2, char1);
    sData = ReplaceStr(sData, "___", char2);
  }
  sData = HmsUtf8Decode(HmsBase64Decode(sData));
  return sData;
} 

///////////////////////////////////////////////////////////////////////////////
// Декодирование ссылок для HTML5 плеера
string Html5Decode(string sEncoded) {
  if ((sEncoded=="") || (Pos(".", sEncoded) > 0)) return sEncoded;
  if (sEncoded[1]=="#") sEncoded = Copy(sEncoded, 2, Length(sEncoded)-1);
  string sDecoded = "";
  for (int i=1; i <= Length(sEncoded); i+=3) {
    sDecoded += "\\u0" + Copy(sEncoded, i, 3);
  }
  return HmsJsonDecode(sDecoded);
  
}
///////////////////////////////////////////////////////////////////////////////
// ---- Создание ссылки на видео ----------------------------------------------
THmsScriptMediaItem AddMediaItem(THmsScriptMediaItem Folder, string sTitle, string sLink, string sGrp='') {
  THmsScriptMediaItem Item = HmsCreateMediaItem(sLink, Folder.ItemID, sGrp); // Создаём ссылку
  Item[mpiTitle     ] = sTitle;       // Наименование
  Item[mpiCreateDate] = DateTimeToStr(IncTime(gStart, 0, 0, -gnTotalItems, 0));
  //  Item[mpiDirectLink] = True;
  // Тут наследуем от родительской папки полученные при загрузке первой страницы данные
  Item.CopyProperties(PodcastItem, [mpiThumbnail, mpiTimeLength, mpiGenre, mpiYear]);
  
  gnTotalItems++;                    // Увеличиваем счетчик созданных элементов
  return Item;                       // Возвращаем созданный объект
}

///////////////////////////////////////////////////////////////////////////////
// --- Создание папки видео ---------------------------------------------------
THmsScriptMediaItem CreateFolder(string sName, string sLink, string sImg='') {
  THmsScriptMediaItem Item = PodcastItem.AddFolder(sLink); // Создаём папку с указанной ссылкой
  Item[mpiTitle     ] = sName;  // Присваиваем наименование
  Item[mpiThumbnail ] = sImg;   // Картинку устанавливаем, которая указана у текущей папки
  Item[mpiCreateDate] = DateTimeToStr(IncTime(gStart, 0, 0, -gnTotalItems, 0));
  
  // Тут наследуем от родительской папки полученные при загрузке первой страницы данные
  Item.CopyProperties(PodcastItem, [mpiThumbnail, mpiTimeLength, mpiGenre, mpiYear]);
  
  gnTotalItems++;               // Увеличиваем счетчик созданных элементов
  return Item;                  // Возвращаем созданный объект
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылки-ошибки
void CreateErrorItem(string sMsg) {
  THmsScriptMediaItem Item = HmsCreateMediaItem('Err', PodcastItem.ItemID);
  Item[mpiTitle     ] = sMsg;
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/icons/symbol-error.png';
}

///////////////////////////////////////////////////////////////////////////////
// ---- Создание информационной ссылки ----------------------------------------
void AddInfoItem(string sTitle) {
  THmsScriptMediaItem Item = HmsCreateMediaItem('-Info'+IntToStr(PodcastItem.ChildCount), PodcastItem.ItemID);
  Item[mpiTitle     ] = HmsHtmlToText(sTitle);  // Наименование (Отображаемая информация)
  Item[mpiTimeLength] = 1;       // Т.к. это псевдо ссылка, то ставим длительность 1 сек.
  Item[mpiThumbnail ] = 'http://wonky.lostcut.net/vids/info.jpg'; // Ставим иконку информации
  Item[mpiCreateDate] = DateTimeToStr(IncTime(gStart, 0, 0, -gnTotalItems, 0));
  gnTotalItems++;
}

///////////////////////////////////////////////////////////////////////////////
// ---- Создание ссылок на файл(ы) по переданной ссылке (шаблону) -------------
void CreateVideoLink(THmsScriptMediaItem Folder, string sName, string sLink, bool bSeparateInFolders=false, int nSeason=0, int nEpisode=0) {
  string sCut, sQualArray, sQual, sFile, sVal; int i, nCount; // Объявляем переменные
  
  // Проверяем, есть ли в переданной ссылке шаблон с массивом существующего качества "[720,480,360]"
  if (HmsRegExMatch('\\[(.*?)\\]', sLink, sQualArray)) {
    sCut   = '['+sQualArray+']';                   // Та часть, которая будет заменятся на индификатор качества
    nCount = WordCount (sQualArray, ',');          // Количество елементов, разделённых запятой
    for (i=1; i<=nCount; i++) {
      sQual = ExtractWord(i, sQualArray, ',');     // Получаем очередной индификатор качества
      if (sQual=='') continue;                     // Может быть пропущен, если не указан
      
      if ((gnQual!=0) && HmsRegExMatch('(\\d+)', sQual, sVal)) {
        if (StrToInt(sVal) < gnQual) continue;     // Фильтрация по установленному качеству
      }
      
      sFile = ReplaceStr(sLink, sCut, sQual);      // Формируем ссылку на файл, заменяя шаблон на индификатор качества
      if (bSeparateInFolders) {                    // Если был передан флаг "Группировать файлы качества по разным папкам",
        AddMediaItem(Folder, sName, sFile, sQual); // то передаём индификатор качества как имя группы, где будет создана ссылка
      } else {                                     
        if (sName=='') HmsRegExMatch('.*/(.*)', sLink, sName); // Получаем имя файла из ссылки (всё что идёт после последнего слеша)
        sName = ReplaceStr(sName, sCut, '');          // Убираем перечисление качества из имени
        sName = ReplaceStr(sName, '_', '');           // А также подчекривания (лишние)
        AddMediaItem(Folder, sQual+' '+sName, sFile); // Добавляем индификатор качества к началу имени и создаём ссылку
      }
    }
    
  } else {
    // Если шаблона выбора качества в ссылке нет, то просто создаём ссылку
    if (sName=='') HmsRegExMatch('.*/(.*)', sLink, sName); // Если имя пустое, получаем имя файла из ссылки (всё что идёт после последнего слеша)
      AddMediaItem(Folder, sName, sLink);                    
    
  }
}

///////////////////////////////////////////////////////////////////////////////
// ---- Создание серий из плейлиста -------------------------------------------
void CreateSeriesFromPlaylist(THmsScriptMediaItem Folder, string sLink, string sName='') {
  string sData, s1, s2, s3; int i; TJsonObject JSON, PLITEM; TJsonArray PLAYLIST; // Объявляем переменные
  int nSeason=0, nEpisode=0; string sVal; THmsScriptMediaItem Item;
  
  // Если передано имя плейлиста, то создаём папку, в которой будем создавать элементы
  if (Trim(sName)!='') Folder = Folder.AddFolder(sName);
  if (HmsRegExMatch('Сезон\\s*(\\d+)', sName, sVal)) nSeason = StrToInt(sVal);
  
  // Если в переменной sLink сожержится знак '{', то там не ссылка, а сами данные Json
  if (Pos('{', sLink)>0) {
    sData = sLink;
  } else {
    sData = HmsDownloadURL(sLink, "Referer: "+gsHeaders); // Загружаем плейлист
    if (LeftCopy(sData, 1)=="#")
      sData = Html5Decode(sData);               // Дешифруем
    else
      sData = DecodeUppodText(sData);           // Дешифруем
  }  
  
  JSON  = TJsonObject.Create();                 // Создаём объект для работы с Json
  try {
    JSON.LoadFromString(sData);                 // Загружаем json данные в объект
    PLAYLIST = JSON.A['playlist'];              // Пытаемся получить array с именем 'playlist'
    if (PLAYLIST==nil) PLAYLIST = JSON.AsArray; // Если массив 'playlist' получить не получилось, то представляем все наши данные как массив
    if (PLAYLIST!=nil) {                        // Если получили массив, то запускаем обход всех элементов в цикле
      for (i=0; i<PLAYLIST.Length; i++) {
        PLITEM = PLAYLIST[i];                   // Получаем текущий элемент массива
        sName = PLITEM.S['comment'];            // Название - значение поля comment
        sLink = PLITEM.S['file'   ];            // Получаем значение ссылки на файл

        if (HmsRegExMatch('Серия\\s*(\\d+)', sName, sVal)) nEpisode = StrToInt(sVal);
        // Форматируем числовое представление серий в названии
        // Если в названии есть число, то будет в s1 - то, что стояло перед ним, s2 - само число, s3 - то, что было после числа
        if (HmsRegExMatch3('^(.*?)(\\d+)(.*)$', sName, s1, s2, s3)) 
          sName = Format('%s %.2d %s', [s1, StrToInt(s2), s3]); // Форматируем имя - делаем число двухцифровое (01, 02...)
        
        // Проверяем, если это вложенный плейлист - запускаем создание элементов из этого плейлиста рекурсивно
        if (PLITEM.B['playlist']) 
          CreateSeriesFromPlaylist(Folder, PLITEM.S['playlist'], sName);
        else
          CreateVideoLink(Folder, sName, sLink, true, nSeason, nEpisode); // Иначе просто создаём ссылки на видео
      }
    } // end if (PLAYLIST!=nil) 
    
  } finally { JSON.Free; }                      // Какие бы ошибки не случились, освобождаем объект из памяти
}

///////////////////////////////////////////////////////////////////////////////
int HexToInt(string sVal) {
  int i, m, r=0; char c;
  for (i=Length(sVal); i>0; i--) {
    c = Uppercase(sVal[i]); m = Round(exp(ln(16) * (Length(sVal)-i)));
    if      (c in ['1'..'9']) r += (Ord(c)-48) * m;
    else if (c in ['A'..'F']) r += (Ord(c)-55) * m;
  }
  return r;
}

///////////////////////////////////////////////////////////////////////////////
// Значения переменных из /templates/Filmix/media/vendor/vendor.js?v2.2.5a
string getDataPlayer(string meta_keys) { 
  int p=13,n=11,m=7,km=18,kn=27,pk=0,dp=29,dn=25,dm=22; 
  string sKey, sVal, sSelectedKey; int i, nVal, nMax=0;
  
  meta_keys = ReplaceStr(meta_keys, ' ', '');
  meta_keys = ReplaceStr(meta_keys, "'", '');
  for (i=0; i<WordCount(meta_keys, ','); i++) {
    sKey = ExtractWord(i+1, meta_keys, ',');
    switch (i) {
      case 0: { sVal= Copy(sKey, p+1, 2)+Copy(sKey, pk+1, 3)+Copy(sKey, dp+1, 1);  }
      case 1: { sVal= Copy(sKey, n+1, 3)+Copy(sKey, kn+1, 1)+Copy(sKey, dn+1, 2);  }
      case 2: { sVal= Copy(sKey, m+1, 1)+Copy(sKey, km+1, 3)+Copy(sKey, dm+1, 2);  }
    }
    nVal = HexToInt(sVal);
    nMax = max(nMax, nVal);
    if (nMax==nVal) sSelectedKey = sKey;
  }
  return sSelectedKey;
}

///////////////////////////////////////////////////////////////////////////////
// Создание ссылок на видео-файл или серии
void CreateLinks() {
  String sHtml, sData, sName, sLink, sVal, sID, sServ, sPage, sPost, sKey;
  THmsScriptMediaItem Item; TRegExpr RegExp; int i, nCount, n; TJsonObject JSON, TRANS;
  sHtml = HmsDownloadURL(mpFilePath, "Referer: "+gsHeaders);  // Загружаем страницу сериала
  sHtml = HmsUtf8Decode(HmsRemoveLineBreaks(sHtml));
  
  if (!HmsRegExMatch('data-id="(\\d+)"', sHtml, sID)) {
    HmsLogMessage(1, "Невозможно найти видео ID на странице фильма.");
    return;
  };
  HmsRegExMatch('//(.*)', gsUrlBase, sServ);
  
  if (HmsRegExMatch('--quality=(\\d+)', mpPodcastParameters, sVal)) gnQual = StrToInt(sVal);
  
  //POST http://filmix.cc/api/episodes/get?post_id=103435&page=1  // episodes name
  //POST http://filmix.cc/api/torrent/get_last?post_id=103435     // tottent file info
  
  // -------------------------------------------------
  // Собираем информацию о фильме
  if (HmsRegExMatch('Время:(.*?)<br', sHtml, sData)) {
    if (HmsRegExMatch('(\\d+)\\s+мин', ' '+sData, sVal)) {
      gnTime = StrToInt(sVal)*60+120; // Из-за того что серии иногда длинее, добавляем пару минут
    }
    PodcastItem[mpiTimeLength] = gnTime;
  }
  HmsRegExMatch('/year-(\\d{4})"', sHtml, PodcastItem[mpiYear]);
  if (HmsRegExMatch('(<a[^>]+genre.*?)</div>', sHtml, sVal)) PodcastItem[mpiGenre] = HmsHtmlToText(sVal);
  // -------------------------------------------------

  int nPort = 80; if (gbHttps) nPort = 443;

  gsSeriesInfo = HmsSendRequestEx(sServ, '/api/episodes/get', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', gsHeaders, 'post_id='+sID, nPort, 16, '', true);
  
  //HmsRegExMatch("meta_key\\s*=\\s*\\[(.*?)\\]", sHtml, sVal);
  //sKey = getDataPlayer(sVal);
  sData = HmsSendRequestEx(sServ, '/api/movies/player_data', 'POST', 'application/x-www-form-urlencoded; charset=UTF-8', gsHeaders, 'post_id='+sID, nPort, 16, sVal, true);
  
  JSON  = TJsonObject.Create();
  try {
    bool bHtml5 = true;
    JSON.LoadFromString(sData);
    TRANS  = JSON['message\\translations\\html5'];
    if (TRANS.Count == 0) {
      TRANS  = JSON['message\\translations\\flash'];
      bHtml5 = false;
    }
    nCount = TRANS.Count;
    for (i=0; i<nCount; i++) {
      sName = TRANS.Names[i];
      sLink = TRANS.S[sName];
      if (bHtml5) sLink = Html5Decode(sLink);
      else        sLink = DecodeUppodText(sLink);
      sLink = HmsExpandLink(sLink, gsUrlBase);

      if (HmsRegExMatch('/pl/', sLink, '')) {
        
        if (nCount > 1) {
          Item = CreateFolder(sName, sLink);
          Item[mpiSeriesInfo] = gsSeriesInfo;
        } else
          CreateSeriesFromPlaylist(PodcastItem, sLink);
        
      } else {
        CreateVideoLink(PodcastItem, sName, sLink); // Это не плейлист - просто создаём ссылки на видео
      }
      
    }
    if (nCount==0) CreateErrorItem('Видео не доступно');
    
  } finally { JSON.Free; }
  
  // Добавляем ссылку на трейлер, если есть
  if (HmsRegExMatch('data-id="trailers"><a[^>]+href="(.*?)"', sHtml, sLink)) {
     Item = AddMediaItem(PodcastItem, 'Трейлер', sLink); // Это не плейлист - просто создаём ссылки на видео
     Item[mpiTimeLength] = 4 * 60;
  }

  // Создаём информационные элементы (если указан ключ --addinfoitems в дополнительных параметрах)
  if (Pos('--addinfoitems', mpPodcastParameters) > 0) {
    if (HmsRegExMatch('(<div[^>]+contry.*?)</div'   , sHtml, sName)) AddInfoItem(HmsHtmlToText(sName));
    //if (HmsRegExMatch('(<div[^>]+directors.*?)</div', sHtml, sName)) AddInfoItem(HmsHtmlToText(sName));
    if (HmsRegExMatch('(Жанр:</span>.*?)</div'      , sHtml, sName)) AddInfoItem(HmsHtmlToText(sName));
    if (HmsRegExMatch('(<div[^>]+translate.*?)</div', sHtml, sName)) AddInfoItem(HmsHtmlToText(sName));
    if (HmsRegExMatch('(<div[^>]+quality.*?)</div'  , sHtml, sName)) AddInfoItem(HmsHtmlToText(sName));
    if (HmsRegExMatch2('<span[^>]+kinopoisk.*?<div.*?>(.*?)</div>.*?<div.*?>(.*?)</div>', sHtml, sName, sVal)) {
      if ((sName!='-') && (sName!='0')) AddInfoItem("КП: "+sName+" ("+sVal+")");
    }
    if (HmsRegExMatch2('<span[^>]+imdb.*?<div.*?>(.*?)</div>.*?<div.*?>(.*?)</div>', sHtml, sName, sVal)) {
      if ((sName!='-') && (sName!='0')) AddInfoItem("IMDB: "+sName+" ("+sVal+")");
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//                      Г Л А В Н А Я   П Р О Ц Е Д У Р А                    //
///////////////////////////////////////////////////////////////////////////////
{
  if (PodcastItem.IsFolder) {
    // Если это папка, создаём ссылки внутри этой папки
    if (HmsRegExMatch('/pl/', mpFilePath, '')) {
      gsSeriesInfo = PodcastItem[mpiSeriesInfo];
      CreateSeriesFromPlaylist(PodcastItem, mpFilePath);
    } else
      CreateLinks();
  
  } else {
    // Если это запустили файл на просмотр, присваиваем MediaResourceLink значение ссылки на видео-файл 
    if (HmsRegExMatch('/(trejlery|trailers)', mpFilePath, '')) {
      gsUserVariable1 =  HmsDownloadURL(mpFilePath, 'Referer: '+mpFilePath, True);
      HmsRegExMatch('video-link[^>]+value="(.*?)"', gsUserVariable1, mpFilePath);
      MediaResourceLink = DecodeUppodText(mpFilePath);
      if (HmsRegExMatch2('(\\[,?(\\w+).*?\\])', MediaResourceLink, gsUserVariable1, gsUserVariable2))
        MediaResourceLink = ReplaceStr(MediaResourceLink, gsUserVariable1, gsUserVariable2);
      
    } else
      MediaResourceLink = mpFilePath;
  
  }
}