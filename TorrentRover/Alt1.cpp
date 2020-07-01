///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //

int gnTotalItems=0; TDateTime gStart=Now;

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

///////////////////////////////////////////////////////////////////////////////
// Функция создания динамической папки с указанным скриптом
THmsScriptMediaItem CreateDynamicItem(THmsScriptMediaItem prntItem, string sTitle, string sLink, string &sScript='') {
  THmsScriptMediaItem Folder = prntItem.AddFolder(sLink, false, 32);
  Folder[mpiTitle     ] = sTitle;
  Folder[mpiCreateDate] = VarToStr(IncTime(Now,0,-prntItem.ChildCount,0,0));
  Folder[200] = 5;         // mpiFolderType
  Folder[500] = sScript;   // mpiDynamicScript
  Folder[501] = 'JScript'; // mpiDynamicSyntaxType
  Folder[mpiFolderSortOrder] = -mpiCreateDate;
  return Folder;
}

///////////////////////////////////////////////////////////////////////////////
// Замена в тексте загруженного скрипта значения текстовой переменной
void ReplaceVarValue(string &sText, string sVarName, string sNewVal) {
  char sVal, sVal2;
  if (HmsRegExMatch2("("+sVarName+"\\s*?=.*?';)", sText, sVal, sVal2, PCRE_SINGLELINE)) {
    HmsRegExMatch(sVarName+"\\s*?=\\s*?'(.*)'", sVal, sVal2);
    sText = ReplaceStr(sText, sVal, ReplaceStr(sVal , sVal2, sNewVal));
  }
}

///////////////////////////////////////////////////////////////////////////////
// Создание папки ПОИСК (с загрузкой скрипта с форума homemediaserver.ru)
void CreateSearchFolder(string sTitle) {
  string sScript='', sHtml; THmsScriptMediaItem Folder;
  
  // Да да, загружаем скрипт с сайта форума HMS
  sHtml = HmsUtf8Decode(HmsDownloadURL('http://homemediaserver.ru/forum/viewtopic.php?f=15&t=2793&p=17395'));
  HmsRegExMatch('BeginSearchJScript\\*/(.*?)/\\*EndSearchJScript', sHtml, sScript, 1, PCRE_SINGLELINE);
  sScript = HmsHtmlToText(sScript, 1251);
  sScript = ReplaceStr   (sScript, #160, ' ');
  
  // И меняем значения переменных на свои
  //ReplaceVarValue(sScript, 'gsSuggestUrl'      , gsAPIUrl+'videos?q=');
  //ReplaceVarValue(sScript, 'gsSuggestResultCut', '');
  //ReplaceVarValue(sScript, 'gsSuggestRegExp'   , '"name":"(.*?)"');
  //ReplaceVarValue(sScript, 'gsSuggestMethod'   , 'POST');
  //sScript = ReplaceStr(sScript, 'gnSuggestNoUTFEnc  = 0', 'gnSuggestNoUTFEnc  = 1');
  
  Folder = FolderItem.AddFolder('-SearchFolder', true);
  Folder[mpiTitle          ] = sTitle;
  Folder[mpiCreateDate     ] = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0)); gnTotalItems++;
  Folder[mpiFolderSortOrder] = "-mpCreateDate";
  
  CreateDynamicItem(Folder, '"Набрать текст"', '-SearchCommands', sScript);
}

///////////////////////////////////////////////////////////////////////////////
// Добавление папки/подкаста
THmsScriptMediaItem AddItem(THmsScriptMediaItem Folder, string sName, string sLink="", bool bForceFolder=false) {
  if (Trim(sLink)=="") { sLink = sName; bForceFolder = true; }       // Если название не указано, а только ссылка - то это папка
  THmsScriptMediaItem Item  = Folder.AddFolder(sLink, bForceFolder); // Создаём подкаст/папку
  Item[mpiTitle          ]  = sName;
  Item[mpiCreateDate     ]  = VarToStr(IncTime(gStart,0,-gnTotalItems,0,0)); gnTotalItems++;
  Item[mpiFolderSortOrder]  = "-mpCreateDate";
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
// Проверка подтверждения на пересоздания структуры
bool CheckForSure() {
  int nAnsw; string sMsg;
  
  sMsg = 'ПЕРЕСОЗДАНИЕ СТРУКТУРЫ ПОДКАСТА.\n\n'+
         'Удалить существующую структуру, очистить ссылки???\n\n'+
         '"Да"  - будут удалены все вложенные папки и ссылки. Структура будет создана заного.\n'+
         '"Нет" - будет создана структура поверх существующих ссылок.\n'+
         '"Отмена" - не пересоздавать структуру.\n';

  if (FolderItem.HasChildItems) {
    nAnsw = MessageDlg(sMsg, 0, 3+8, 0);
    if (nAnsw == mrCancel) return false;
    if (nAnsw == mrYes   ) FolderItem.DeleteChildItems();
  }
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////
//                      Г Л А В Н А Я   П Р О Ц Е Д У Р А                    //
// ----------------------------------------------------------------------------
{
  THmsScriptMediaItem Item, Folder;

  if (!CheckForSure()) return; // Удаление существующих ссылок

  CreateSearchFolder('00. Поиск');
  
  Folder = AddItem(FolderItem, '01. Локальные папки');
  AddItem(Folder, 'TorrServer'       , '-TorrServer');
  AddItem(Folder, 'Video (.torrents)', 'D:/VIDEO'  ); // Для примера
  
  Folder = AddItem(FolderItem, '02. RuTracker.org', 'http://rutracker37.tk', true);
  AddItem(Folder, 'Зарубежное кино'               , '/forum/viewforum.php?f=7');
  AddItem(Folder, 'Зарубежные сериалы'            , '/forum/viewforum.php?f=189');
  AddItem(Folder, 'Зарубежные сериалы (HD Video)' , '/forum/viewforum.php?f=2366');
  AddItem(Folder, 'Наше кино'                     , '/forum/viewforum.php?f=22');
  AddItem(Folder, 'Русские сериалы'               , '/forum/viewforum.php?f=9');
  AddItem(Folder, 'Арт-хаус и авторское кино'     , '/forum/viewforum.php?f=124');
  AddItem(Folder, 'Сериалы Латинской Америки, Турции и Индии', '/forum/viewforum.php?f=911');
  AddItem(Folder, 'Азиатские сериалы'             , '/forum/viewforum.php?f=2100');
  AddItem(Folder, 'HD Video'                      , '/forum/viewforum.php?f=2198');
  AddItem(Folder, '3D-Стерео Кино, Видео, TV и Спорт', '/forum/viewforum.php?f=352');
  AddItem(Folder, 'Мультфильмы'                   , '/forum/viewforum.php?f=4');
  AddItem(Folder, 'Мультсериалы'                  , '/forum/viewforum.php?f=921');
  AddItem(Folder, 'Футбол'                        , '/forum/viewforum.php?f=1608');
  AddItem(Folder, 'Хоккей'                        , '/forum/viewforum.php?f=2009');
  AddItem(Folder, 'Форум'                         , '/forum/index.php');
  AddItem(Folder, 'Электронная музыка'            , '/forum/index.php?c=23');
  AddItem(Folder, 'Аудиокниги'                    , '/forum/index.php?c=33');
  
  Folder = AddItem(FolderItem, '03. Rutor.org', 'http://rutor.info', true);
  AddItem(Folder, 'Главная'          , '/');
  AddItem(Folder, 'ТОП'              , '/top');
  AddItem(Folder, 'Зарубежные фильмы', '/kino');
  AddItem(Folder, 'Наши фильмы'      , '/nashe_kino');
  AddItem(Folder, 'Сериалы'          , '/seriali');
  AddItem(Folder, 'Мультфильмы'      , '/multiki');
  AddItem(Folder, 'Музыка'           , '/audio');
  AddItem(Folder, 'Категории'        , '/categories');

  Folder = AddItem(FolderItem, '04. nntt.org', 'http://www.nntt.org', true);
  AddItem(Folder, 'Топы лучших фильмов, сериалов...', '/viewforum.php?f=1048');
  AddItem(Folder, 'HD, DVD, 3D фильмы'              , '/viewforum.php?f=4');
  AddItem(Folder, 'Зарубежное кино (Rip)'           , '/viewforum.php?f=5');
  AddItem(Folder, 'Наше кино (Rip)'                 , '/viewforum.php?f=6');
  AddItem(Folder, 'Сериалы'                         , '/viewforum.php?f=270');
  AddItem(Folder, 'Мультфильмы и мультсериалы'      , '/viewforum.php?f=235');
  AddItem(Folder, 'TV, развлекательные телепередачи и шоу, приколы и юмор', '/viewforum.php?f=9');
  AddItem(Folder, 'Документальные фильмы и телепередачи', '/viewforum.php?f=12');
  AddItem(Folder, 'Обучающее видео'                 , '/viewforum.php?f=962');
  AddItem(Folder, 'Музыкальное видео'               , '/viewforum.php?f=1046');
  AddItem(Folder, 'Детский'                         , '/viewforum.php?f=1010');
  AddItem(Folder, 'Аниме с русской озвучкой'        , '/viewforum.php?f=353');
  AddItem(Folder, 'Аниме c оригинальной озвучкой / субтитрами', '/viewforum.php?f=354');
  AddItem(Folder, 'Полнометражное аниме'            , '/viewforum.php?f=355');
  AddItem(Folder, 'Манга'                           , '/viewforum.php?f=356');
  AddItem(Folder, 'Музыка и Клипы'                  , '/viewforum.php?f=357');
  AddItem(Folder, 'Hentai'                          , '/viewforum.php?f=359');
  AddItem(Folder, 'Электронная музыка'              , '/viewforum.php?f=766');
  AddItem(Folder, 'Поп музыка'                      , '/viewforum.php?f=886');
  AddItem(Folder, 'Классическая и Инструментальная музыка', '/viewforum.php?f=913');
  AddItem(Folder, 'Аудиокниги'                      , '/viewforum.php?f=616');
  AddItem(Folder, 'Футбол'                          , '/viewforum.php?f=825');
  AddItem(Folder, 'Авто / мотоспорт'                , '/viewforum.php?f=839');
  AddItem(Folder, 'Баскетбол'                       , '/viewforum.php?f=840');
  AddItem(Folder, 'Хоккей'                          , '/viewforum.php?f=841');
  AddItem(Folder, 'Боевые искусства'                , '/viewforum.php?f=842');
  AddItem(Folder, 'Остальные виды спорта'           , '/viewforum.php?f=843');
  AddItem(Folder, 'Pron'                            , '/viewforum.php?f=1008');
  AddItem(Folder, 'Эротика'                         , '/viewforum.php?f=1055');
  AddItem(Folder, 'Тестовый раздел (для новичков)'  , '/viewforum.php?f=763');
  AddItem(Folder, 'Форум', '/');
  
  Folder = AddItem(FolderItem, '05. Uniongang.org', 'http://uniongang.org', true);
  AddItem(Folder, '4K (UHD)'           , '/browse.php?incldead=1&cat=23');
  AddItem(Folder, '3D'                 , '/browse.php?incldead=1&cat=20');
  AddItem(Folder, 'Фильм / AVI'        , '/browse.php?incldead=1&cat=1');
  AddItem(Folder, 'Фильм / WEB-DL'     , '/browse.php?incldead=1&cat=21');
  AddItem(Folder, 'Фильм / x264'       , '/browse.php?incldead=1&cat=2');
  AddItem(Folder, 'Фильм / DVD5'       , '/browse.php?incldead=1&cat=3');
  AddItem(Folder, 'Фильм / DVD9'       , '/browse.php?incldead=1&cat=4');
  AddItem(Folder, 'Фильм / HDBD'       , '/browse.php?incldead=1&cat=5');
  AddItem(Folder, 'Фильм / Сериал'     , '/browse.php?incldead=1&cat=6');
  AddItem(Folder, 'Фильм / Доку'       , '/browse.php?incldead=1&cat=7');
  AddItem(Folder, 'Фильм / Эротика'    , '/browse.php?incldead=1&cat=8');
  AddItem(Folder, 'Мультфильм'         , '/browse.php?incldead=1&cat=9');
  AddItem(Folder, 'КВН / Юмор'         , '/browse.php?incldead=1&cat=10');
  AddItem(Folder, 'Музыка / Русская'   , '/browse.php?incldead=1&cat=13');
  AddItem(Folder, 'Музыка / Зарубежная', '/browse.php?incldead=1&cat=14');
  AddItem(Folder, 'Видеоклип'          , '/browse.php?incldead=1&cat=15');
  AddItem(Folder, 'Аудиокнига'         , '/browse.php?incldead=1&cat=16');
   
  HmsLogMessage(1, mpTitle+': Создано ссылок - '+IntToStr(gnTotalItems));
}