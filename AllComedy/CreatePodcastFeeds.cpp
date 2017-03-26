// 2017.03.26
///////////////////////  Создание структуры подкаста  /////////////////////////

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
string    gsUrlBase    = "http://allcc.ru"; 
int       gnTotalItems = 0; 
TDateTime gStart       = Now;

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

///////////////////////////////////////////////////////////////////////////////
// Создание папки или подкаста
THmsScriptMediaItem CreateFolder(THmsScriptMediaItem Parent, string sName, string sLink, string sParams='', bool bForceFolder=false) {
  sLink = HmsExpandLink(sLink, gsUrlBase);
  THmsScriptMediaItem Item = Parent.AddFolder(sLink, bForceFolder);
  Item[mpiTitle     ] = sName;
  Item[mpiCreateDate] = IncTime(gStart,0,-gnTotalItems,0,0); gnTotalItems++; // Для обратной сортировки по дате создания
  Item[mpiPodcastParameters] = sParams;
  Item[mpiFolderSortOrder  ] = "-mpCreateDate";
  return Item;
}

///////////////////////////////////////////////////////////////////////////////
//                     Г Л А В Н А Я   П Р О Ц Е Д У Р А                     //
{
  FolderItem.DeleteChildItems();
  
  CreateFolder(FolderItem, '01 Новинки'            , '/');
  CreateFolder(FolderItem, '02 Comedy Club'        , '/comedy-club/video/' );
  CreateFolder(FolderItem, '03 Камеди Батл'        , '/comedy-battle/video-online/');
  CreateFolder(FolderItem, '04 Stand Up'           , '/stand-up/online/'   );
  CreateFolder(FolderItem, '05 ХБ шоу'             , '/hb-show/all-series/');
  CreateFolder(FolderItem, '06 Не спать! на ТНТ'   , '/ne-spat/vipuski/'   );
  CreateFolder(FolderItem, '07 Камеди Вумен'       , '/comedy-woman/vipuski-online/');
  CreateFolder(FolderItem, '08 Незлоб'             , '/nezlob/series-online/');
  CreateFolder(FolderItem, '09 Однажды в России'   , '/odnazhdy-v-rossii/');
  CreateFolder(FolderItem, '10 Импровизация на ТНТ', '/improvizaciya/'    );
  CreateFolder(FolderItem, '11 Открытый микрофон'  , '/otkrytyy-mikrofon/');
  
  HmsLogMessage(1, mpTitle+': Создано элементов - '+IntToStr(gnTotalItems));
}
