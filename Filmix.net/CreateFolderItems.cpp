// 2017.03.30
////////////////////////  Создание  списка  видео   ///////////////////////////

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
THmsScriptMediaItem Podcast = GetRoot(); // Главная папка подкаста
string    gsUrlBase    = 'http://filmix.me';   // Url база ссылок нашего сайта
int       gnTotalItems = 0;                    // Количество созданных элементов
TDateTime gStart       = Now;                  // Время запуска скрипта

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

///////////////////////////////////////////////////////////////////////////////
// Установка переменной Podcast: поиск родительской папки, содержащий скрипт
THmsScriptMediaItem GetRoot() {
  Podcast = FolderItem; // Начиная с текущего элемента, ищется создержащий срипт
  while ((Trim(Podcast[550])=='') && (Podcast.ItemParent!=nil)) Podcast=Podcast.ItemParent;
  return Podcast;
}

///////////////////////////////////////////////////////////////////////////////
// --- Создание папки видео ---------------------------------------------------
THmsScriptMediaItem CreateFolder(THmsScriptMediaItem Folder, string sName, string sLink, string sImg='') {
  THmsScriptMediaItem Item = Folder.AddFolder(sLink); // Создаём папку с указанной ссылкой
  Item[mpiTitle     ] = sName; // Присваиваем наименование
  Item[mpiThumbnail ] = sImg;  // Картинка
  Item[mpiCreateDate] = DateTimeToStr(IncTime(gStart, 0, 0, -gnTotalItems, 0));
  gnTotalItems++;             // Увеличиваем счетчик созданных элементов
  return Item;                // Возвращаем созданный объект
}

///////////////////////////////////////////////////////////////////////////////
// ------------------------------------ Получение название группы из имени ----
string GetGroupName(string sName) {
  string sGrp = '#';
  if (HmsRegExMatch('([A-ZА-Я0-9])', sName, sGrp, 1, PCRE_CASELESS)) sGrp = Uppercase(sGrp);
  if (HmsRegExMatch('[0-9]', sGrp, sGrp)) sGrp = '#';
  if (HmsRegExMatch('[A-Z]', sGrp, sGrp)) sGrp = 'A..Z';
  return sGrp;
}

///////////////////////////////////////////////////////////////////////////////
// --- Создание структуры -----------------------------------------------------
void CreateVideoFolders() {
  string sHtml, sData, sName, sLink, sImg, sYear, sVal, sGroupingKey, sGrp; 
  int i, nPages, nMaxInGroup, iCnt, nGrp; TRegExpr RegEx;           // Объявляем переменные
  THmsScriptMediaItem Folder = FolderItem;

  sHtml  = ''; // Текст загруженных страниц сайта
  nPages = 8;  // Количество загружаемых страниц
  nMaxInGroup = 100;
  
  HmsRegExMatch('--group=(\\w+)', mpPodcastParameters, sGroupingKey);
  if (HmsRegExMatch('--pages=(\\d+)', mpPodcastParameters, sVal)) nPages = StrToInt(sVal);
  
  // Проверяем, не заняться ли нам поиском?
  if (LeftCopy(mpFilePath, 4)!='http') {
    // Если в поле "Ссылка" нет реальной ссылки, то делаем ссылку сами - будем искать наименование
    mpFilePath = 'http://filmix.me/engine/ajax/sphinx_search.php?search_start=0&do=search&subaction=search&sort_name=&undefined=asc&sort_date=&sort_favorite=&searchSortby=&no_trailer=on&story='+HmsHttpEncode(HmsUtf8Encode(mpTitle));
    sHtml  = HmsUtf8Decode(HmsDownloadUrl(mpFilePath)); // Загружаем страницу
    nPages = 0; // Это чтобы не загружались следующие страницы (загружать так их уже не получится)
  }

  // Загружаем первые сколько-то страниц
  for (i=1; i<nPages; i++) {
    HmsSetProgress(Trunc(i*100/nPages));                                         // Устанавливаем позицию прогресса загрузки 
    HmsShowProgress(Format('%s: Страница %d из %d', [mpTitle, i, nPages]));      // Показываем окно прогресса
    sHtml += HmsUtf8Decode(HmsDownloadUrl(mpFilePath+'/page/'+IntToStr(i)+'/')); // Загружаем страницу
    if (HmsCancelPressed()) break;      // Если в окне прогресса нажали "Отмена" - прерываем цикл
  }
  HmsHideProgress();                    // Убираем окно прогресса с экрана
  sHtml = HmsRemoveLineBreaks(sHtml);   // Удаляем переносы строк, для облегчения работы с регулярными выражениями

  // Создаём объект для поиска и ищем в цикле по регулярному выражению
  RegEx = TRegExpr.Create('<article(.*?)</article>'); 
  try {
    if (RegEx.Search(sHtml)) do {       // Если нашли совпадение, запускаем цикл
      sLink=''; sName=''; sImg=''; sYear=''; // Очищаем переменные от предыдущих значений
      
      HmsRegExMatch('<a[^>]+href="(.*?)"'     , RegEx.Match, sLink); // Ссылка
      HmsRegExMatch('(<div[^>]+name.*?</(h\\d|div)>)', RegEx.Match, sName); // Наименование
      HmsRegExMatch('<img[^>]+src="(.*?)"'    , RegEx.Match, sImg ); // Картинка
      HmsRegExMatch('year.*?(\\d{4})'         , RegEx.Match, sYear); // Год

      if (sLink=='') continue;          // Если нет ссылки, значит что-то не так
       
      sLink = HmsExpandLink(sLink, gsUrlBase);             // Делаем ссылку полной, если она таковой не является
      if (sImg!='') sImg = HmsExpandLink(sImg, gsUrlBase); // Если есть ссылка на картинку, делаем ссылку полной        
      sName = HmsHtmlToText(sName);                        // Преобразуем html в простой текст
      HmsRegExMatch('(.*?)/' , sName, sName);              // Обрезаем слишком длинные названия (на англ. языке)

      // Если в названии нет года, добавляем год выхода 
      if ((sYear!='') && (Pos(sYear, sName)<1)) sName += ' ('+sYear+')';

      if      (sGroupingKey=="quant") {
        sGrp = Format("%.2d", [nGrp]);
        iCnt++; if (iCnt>=nMaxInGroup) { nGrp++; iCnt=0; }
      } 
      else if (sGroupingKey=="alph") sGrp = GetGroupName(sName);
      else if (sGroupingKey=="year") sGrp = sYear;
      else sGrp = "";

      if (Trim(sGrp)!="") Folder = CreateFolder(FolderItem, sGrp, sGrp);
      
      CreateFolder(Folder, sName, sLink, sImg); // Вызываем функцию создания папки видео
                                      
    } while (RegEx.SearchAgain);        // Повторяем цикл, если найдено следующее совпадение
  
  } finally { RegEx.Free; }             // Освобождаем объект из памяти

  if      (sGroupingKey=="alph") FolderItem.Sort("mpTitle");
  else if (sGroupingKey=="year") FolderItem.Sort("-mpTitle");
} 

///////////////////////////////////////////////////////////////////////////////
// Проверка и обновление скриптов подкаста
void CheckPodcastUpdate() {
  string sData, sName, sLang, sExt, sMsg; int i, mpiTimestamp=100602, mpiSHA, mpiScript;
  TJsonObject JSON, JFILE; TJsonArray JARRAY; bool bChanges=false;
  
  // Если после последней проверки прошло меньше получаса - валим
  if ((Trim(Podcast[550])=='') || (DateTimeToTimeStamp1970(Now, false)-StrToIntDef(Podcast[mpiTimestamp], 0) < 1800)) return;
  Podcast[mpiTimestamp] = DateTimeToTimeStamp1970(Now, false); // Запоминаем время проверки
  sData = HmsDownloadURL('https://api.github.com/repos/WendyH/HMS-podcasts/contents/Filmix.net', "Accept-Encoding: gzip, deflate", true);
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
//                    Г Л А В Н А Я    П Р О Ц Е Д У Р А                     //
///////////////////////////////////////////////////////////////////////////////
{
  FolderItem.DeleteChildItems(); // Удаляем созданное ранее содержимое
  
  if ((Pos('--nocheckupdates', mpPodcastParameters)<1) && (mpComment=='--update')) CheckPodcastUpdate(); // Проверка обновлений подкаста
  
  CreateVideoFolders();          // Запускаем загрузку страниц и создание папок видео
  
  HmsLogMessage(1, Podcast[mpiTitle]+' "'+mpTitle+'": Создано элементов - '+IntToStr(gnTotalItems));
}