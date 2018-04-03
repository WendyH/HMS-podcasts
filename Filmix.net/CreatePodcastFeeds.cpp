// 2018.04.04  Collaboration: WendyH, Big Dog, михаил
///////////////////////  Создание структуры подкаста  /////////////////////////

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
string gsUrlBase = 'http://filmix.nl'; // Url база ссылок нашего сайта

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

///////////////////////////////////////////////////////////////////////////////
// Создание подкаста
THmsScriptMediaItem CreatePodcast(THmsScriptMediaItem Folder, string sName, string sLink, String sParams='') {
  THmsScriptMediaItem Item;                // Объявляем переменные
  sLink = HmsExpandLink(sLink, gsUrlBase); // Делаем ссылку полной, если она таковой не является
  Item  = Folder.AddFolder(sLink);         // Создаём подкаст с указанной ссылкой
  Item[mpiTitle] = sName;                  // Присваиваем наименование
  Item[mpiPodcastParameters] = sParams;    // Дополнительные параметры подкаста
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Удаление всех существующих разделов(перед созданием) кроме поиска
void DeleteFolders() {
  THmsScriptMediaItem Item, FavFolder; int i, nAnsw;
  for (i=FolderItem.ChildCount-1; i>=0; i--) {
    Item = FolderItem.ChildItems[i]; if (Item[mpiFilePath]=='-SearchFolder') continue;
    Item.Delete();
  }
}

///////////////////////////////////////////////////////////////////////////////
// Создание структуры
void CreateStructure() {
  string sHtml, sData, sName, sLink; TRegExpr RegEx;         // Объявляем переменные
  THmsScriptMediaItem Folder, Item;

  Folder = FolderItem.AddFolder('-SearchFolder', true);      // Создаём папку
  Folder[mpiTitle] = '0 Поиск';

  CreatePodcast(FolderItem, '1 Последние поступления', '/'); // Создаём подкаст
  CreatePodcast(FolderItem, '2 Новые фильмы'         , '/films'); // Создаём подкаст
  CreatePodcast(FolderItem, '3 Популярные фильмы'    , '/popular/films'); // Создаём подкаст
  CreatePodcast(FolderItem, '4 Мультфильмы'          , '/multfilmy');
  CreatePodcast(FolderItem, '5 Мультсериалы'         , '/multserialy', '--group=alph --pages=10');
  CreatePodcast(FolderItem, '6 Сериалы'              , '/serialy'    , '--group=alph --pages=50');
  
  Folder = FolderItem.AddFolder('7 По категориям', true);    // Создаём папку
  
  sHtml = HmsUtf8Decode(HmsDownloadUrl(gsUrlBase));  // Загружаем страницу
  sHtml = HmsRemoveLineBreaks(sHtml);                // Удаляем переносы строк

  // Вырезаем нужный блок в переменную sData
  HmsRegExMatch('menu-title">Фильмы<.*?</ul>(.*?)class="lucky"', sHtml, sData);

  // Создаём объект для поиска текста и ищем в цикле по регулярному выражению
  RegEx = TRegExpr.Create('<a[^>]+href="(.*?)".*?</a>'); 
  try {
    if (RegEx.Search(sData)) do {     // Если нашли совпадение, запускаем цикл 
        sLink = RegEx.Match(1);       // Получаем значение первой группировки 
        sName = RegEx.Match(0);       // Получаем совпадение всего шаблона 
        sName = HmsHtmlToText(sName); // Преобразуем html в простой текст 

        CreatePodcast(Folder, sName, sLink); // Создаём подкаст с полученным именем и ссылкой
                                      
    } while (RegEx.SearchAgain);      // Повторяем цикл, если найдено следующее совпадение
  
  } finally { RegEx.Free(); }         // Освобождаем объект из памяти

} 

///////////////////////////////////////////////////////////////////////////////
//                  Г Л А В Н А Я    П Р О Ц Е Д У Р А                       //
{
  DeleteFolders();   // Удаляем созданное ранее содержимое
  CreateStructure(); // Создаём подкасты
}