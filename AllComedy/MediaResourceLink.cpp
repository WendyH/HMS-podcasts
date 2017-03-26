// VERSION = 2017.03.26
//////////////////////  Получение ссылки на поток   ///////////////////////////

///////////////////////////////////////////////////////////////////////////////
//               Г Л О Б А Л Ь Н Ы Е   П Е Р Е М Е Н Н Ы Е                   //
string gsUrlBase = "http://allcc.ru";

///////////////////////////////////////////////////////////////////////////////
//                             Ф У Н К Ц И И                                 //

////////////////////////////////////////////////////////////////////////////////
// Получение ссылки на rutube.ru
void GetLink_RuTube(string sLink) {
  string sData, sImg, sVideoId = ''; TJsonObject JSON, BALANCER;
  
  // Восполняем неуказанный протол http, если нужно
  if (LeftCopy(sLink, 2)=='//'  ) sLink = 'http:'   + Trim(sLink);
  if (LeftCopy(sLink, 4)!='http') sLink = 'http://' + Trim(sLink);
  
  HmsRegExMatch('video.rutube.ru/([\\w\\d]{20,})', sLink, sVideoId);
  HmsRegExMatch('rutube.ru/video/([\\w\\d]{20,})', sLink, sVideoId);
  if (sVideoId == '') {
    sData = HmsDownloadUrl(sLink, 'Referer: '+mpFilePath, true);
    HmsRegExMatch('video.rutube.ru/([\\w\\d]{20,})', sData, sVideoId);
    HmsRegExMatch('rutube.ru/video/([\\w\\d]{20,})', sData, sVideoId);
  }
  if (sVideoId != '') {
    sLink = 'http://rutube.ru/api/play/options/' + sVideoId;
    sLink+= '?format=json&no_404=true&sqr4374_compat=1&referer=' + HmsHttpEncode(mpFilePath) + '&_='+ReplaceStr(VarToStr(random), ',', '.');
    sData = HmsDownloadUrl(sLink, 'Referer: '+mpFilePath, true);
    if (HmsRegExMatch('(http[^">\']+m3u8[^"}>\']+)', sData, sLink)) {
      MediaResourceLink = ' '+sLink;
    } else {
      JSON  = TJsonObject.Create();
      try {
        JSON.LoadFromString(sData);
        sImg = JSON.S["thumbnail_url"];
        if (sImg!='') PodcastItem[mpiThumbnail] = sImg;
        BALANCER = JSON.O["video_balancer"];
        if (BALANCER!=nil) {
          sLink = BALANCER.S["json"];
          if (sLink!='') {
            sData = HmsDownloadUrl(sLink, 'Referer: '+mpFilePath, true);
            HmsRegExMatch('(http[^">\']+)', sData, MediaResourceLink);
          }
        }
      } finally { JSON.Free; }
    }
  }
  if (MediaResourceLink=='')
    HmsLogMessage(2, mpTitle+': не удалось получить ссылку на видео rutube.ru по '+sLink);
}

///////////////////////////////////////////////////////////////////////////////
// Получение ссылки на поток с сайта allcc.ru
void GetLink_AllccRu() {
  string sHtml, sLink;
  
  sHtml = HmsDownloadURL(mpFilePath, 'Accept-Encoding: gzip, deflate', true);
  if (!HmsRegExMatch('<iframe[^>]src="(.*?)"', sHtml, sLink)) {
    HmsLogMessage(2, mpTitle+": Не удалось получить ссылку на поток с allcc.ru");
    return;
  }
  if (Pos('rutube', sLink) > 0) GetLink_RuTube(sLink);
  else {
    HmsLogMessage(2, mpTitle+": Не удалось определить тип ссылки видео с allcc.ru");
    return;
  }
}


///////////////////////////////////////////////////////////////////////////////
//                     Г Л А В Н А Я   П Р О Ц Е Д У Р А                     //
{
  GetLink_AllccRu();
}
