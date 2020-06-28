// JSON-кофигурация правил поика торрент-файлов на сайтах
// https://github.com/WendyH/HMS-podcasts/blob/master/TorrentRover/Config.md
{
  /////////////////////////////////////////////////////////////////////////////
  // Массив сайтов для поиска
  "Search": [
    {
      "Site"   : "uniongang.org",
      "Enable" : 1,
      "Url"    : "http://uniongang.org/browse.php?search=<TITLE>",
      "Utf8"   : 0,
      "Method" : "GET",
      "Block"  : "(<a[^>]+href=\"/torrent-.*?</a>).*?</tr>",
      "Title"  : "(<a.*</a>)",
      "Link"   : "<a[^>]+href=\"(.*?)\"",
    },
    {
      "Site"   : "rutracker37.tk",
      "Enable" : 1,
      "Url"    : "http://rutracker37.tk/forum/tracker.php?nm=<TITLE>",
      "Utf8"   : 1,
      "Method" : "POST",
      "Block"  : "(<a[^>]+data-topic_id.*?</a>)",
      "Title"  : "(<a.*?)/?</a>",
      "Link"   : "<a[^>]+href=\"(.*?)\"",
    },
    {
      "Site"   : "rutor.info",
      "Enable" : 1,
      "Url"    : "http://rutor.info/search/0/0/000/0/<TITLE>",
      "Utf8"   : 1,
      "Block"  : "(<tr[^>]+(tum|gai).*?</tr>)",
      "Title"  : "(<a[^>]+href=\"/torrent.*?</a>)",
      "Link"   : "<a[^>]+href=\"(/torrent.*?)\"",
      "PHProxy": "https://hms.lostcut.net/proxy.php",
    },
  ],
  /////////////////////////////////////////////////////////////////////////////
  // Набор правил для сайтов (имя объекта - домен)
  "rutracker37.tk": {
    "Login": {
      "Url"       : "https://rutracker37.tk/forum/login.php",
      "Method"    : "POST",
      "PostData"  : "login_username=<LOGIN>&login_password=<PASSW>&login=%C2%F5%EE%E4",
      "NoRedirect": 1,
      "NoCache"   : 1,
      "Success"   : "(302 Moved|302 Found|logout)"
    },
    "LoginCondition": "login-form-quick",
    "Conditions": [
      {
        "ConditionLink": "viewtopic.php\\?t=\\d+",
        "Rule": "Torrent"
      },
      {
        "ConditionText": "(forumlink|<a[^>]+id=\"tt-)",
        "Block"   : "(<h\\d[^>]+forumlink.*?</h\\d>|<a[^>]+id=\"tt-.*?</a>)",
        "Title"   : "(<a.*</a>)",
        "Link"    : "<a[^>]+href=\"(.*?)\"",
        "NextPage": ".*<a[^>]+class=\"pg\"[^>]+href=\"([^\"]+)\">След.</a>"
      }
    ],
    "Torrent": {
      "Url"     : "(dl.php\\?t=\\d+)",
      "Method"  : "POST",
      "VARS"    : { "TOKEN": "form_token\\s*?:\\s*?[\"'](.*?)[\"']" },
      "PostData": "form_token=<TOKEN>",
      "Error"   : "(<p[^>]+attach_link.*?</p>)",
      "Seeders" : "class=\"seed\">.*?(\\d+)",
      "Leechers": "class=\"leech\">.*?(\\d+)",
      "Date"    : "Когда зарегистрирован.*?>(.*?)<",
      "Size"    : "tor-size-humn.*?>(.*?)<",
      "Image"   : "postImgAligned[^>]+title=\"(http[^\"]+(jpg|png))\"",
      "Year"    : "Год выпуска.*?(\\d{4})",
      "Duration": "(?:Время звучания|Продолжительность)(.*?)<br",
      "Genre"   : "Жанр.*?:(.*?)<",
      "Subs"    : "Субтитры.*?:(.*?)<br",
      "Text"    : "(<div[^>]+post_body.*?)<div[^>]+sp-wrap"
    }
  },
  // --------------------------------------------------------------------------
  "rutor.info": {
    "PHProxy": "https://hms.lostcut.net/proxy.php",
    "Conditions": [
      {
        "ConditionLink": "/torrent/\\d+",
        "Rule" : "Torrent"
      },
      {
        "ConditionText": "<h2><a href=\"/\\w+\"",
        "Block": "(<h2><a[^>]+href=\"/\\w+\".*?</h2>)",
        "Title": "(<a.*?</a>)",
        "Link" : "<a[^>]+href=\"(.*?)\""
      },
      {
        "ConditionText": "<tr[^>]+(tum|gai)",
        "Block": "(<tr[^>]+(tum|gai).*?</tr>)",
        "Title": "(<a[^>]+href=\"/torrent.*?</a>)",
        "Link" : "<a[^>]+href=\"(/torrent.*?)\"",
        "NextPage": "<a[^>]+href=\"([^\"]+)\"><b>След."
      }
    ],
    "Torrent": {
      "Url"     : "(/download/\\d+)",
      "Seeders" : "Раздают</td><td>(.*?)<",
      "Leechers": "Качают</td><td>(.*?)<",
      "Date"    : "Добавлен</td><td>(.*?)<",
      "Size"    : "Размер</td><td>(.*?)<",
      "Image"   : "details.*?<img[^>]+src=['\"]([^'\"]+radikal.*?)['\"]",
      "Year"    : "Год выпуска.*?(\\d{4})",
      "Duration": "Продолжительность.*?>(.*?)<br",
      "Genre"   : "Жанр:?<[^>]+>:?(.*?)<",
      "Text"    : "(<table[^>]+details.*?)(<div[^>]+hidewrap|</table>)"
    }
  },
  // --------------------------------------------------------------------------
  "www.nntt.org": {
    "PHProxy": "https://hms.lostcut.net/proxy.php",
    "Conditions": [
      {
        "ConditionText": "/download/file.php\\?id=\\d+",
        "Rule": "Torrent"
      },
      {
        "ConditionText": "<a[^>]+cat_title[^>]+href=\"http",
        "Block": "(<a[^>]+cat_title[^>]+href=\"http.*?</a>)",
        "Title": "(<a.*?</a>)",
        "Link" : "<a[^>]+href=\"(.*?)\""
      },
      {
        "ConditionText": "<a[^>]+forumlink",
        "Block": "(<a[^>]+forumlink.*?</a>)",
        "Title": "(<a.*?</a>)",
        "Link" : "<a[^>]+href=\"(.*?)\""
      },
      {
        "ConditionText": "<a[^>]+topictitle",
        "Block": "(<a[^>]+topictitle.*?</a>)",
        "Title": "(<a.*?</a>)",
        "Link" : "<a[^>]+href=\"(.*?)\"",
        "NextPage": ".*<a[^>]+href=\"([^\"]+start=\\d+)\">След"
      },
      {
        "ConditionText": "<a[^>]+href=\"[^\"]*viewtopic\\.php\\?f=\\d",
        "Block": "(<a[^>]+viewtopic.php\\?f=\\d.*?</a>)",
        "Title": "(<a.*?</a>)",
        "Link" : "<a[^>]+href=\"(.*?)\"",
      }
    ],
    "Torrent": {
      "Url"     : "(/download/file.php\\?id=\\d+)",
      "Error"   : "<td class=\"c1 border_lr p6 gen\">(.*?)</td>",
      "Seeders" : "Сидеров.*?>(.*?)</span",
      "Leechers": "Личеров.*?>(.*?)</span",
      "Date"    : "Добавлен.*?>(.*?)<",
      "Size"    : "Размер:.*?>(.*?)<",
      "Image"   : "postbody.*?<img[^>]+src=[\"'](.*?)[\"']",
      "Year"    : "Год выпуска:.*?(\\d{4})",
      "Duration": "Продолжительность:?(<.*?)<br",
      "Genre"   : ">Жанр:?(<.*?)<br",
      "Text"    : "postbody.*?>(.*?)<div[^>]+(sp-wrap|sp-body)"
    }
  },
  // --------------------------------------------------------------------------
  "uniongang.org": {
    "LoginCondition": "/takelogin.php",
    "Login": {
      "Url": "http://uniongang.org/takelogin.php",
      "PostData"  : "username=<LOGIN>&password=<PASSW>",
      "Method"    : "POST",
      "NoRedirect": 1,
      "NoCache"   : 1,
      "Success"   : "302 Moved"
    },
    "Conditions": [
      {
        "ConditionLink": "/torrent-\\d+",
        "Rule"    : "Torrent"
      },
      {
        "ConditionText": "(/torrent-\\d+)",
        "Block"   : "<tr>.*?(<a[^>]+href=\"/torrent-.*?</a>)",
        "Title"   : "(<a.*</a>)",
        "Link"    : "<a[^>]+href=\"(/torrent-.*?)\"",
        "NextPage": ".*<a[^>]+href=\"(browse.php[^\"]+page=\\d+)"
      }
    ],
    "Torrent": {
      "Subs"    : "Субтитры:(.*?)<",
      "Url"     : "(download.php\\?id=\\d+)",
      "Method"  : "GET",
      "Error"   : "(<p[^>]+attach_link.*?</p>)",
      "Seeders" : "(\\d+)\\s+раздающих",
      "Leechers": "(\\d+)\\s+качающих",
      "Date"    : "Когда зарегистрирован.*?>(.*?)<",
      "Size"    : ">([\\d\\.]+\\s+GB)",
      "Image"   : "Описание.*?<img[^>]+src=\"(http[^\"]+(jpg|png))\"",
      "Year"    : "Год выхода.*?(\\d{4})",
      "Duration": "(?:Время звучания|Продолжительность)(.*?)<br",
      "Genre"   : "<b>Жанр:(.*?)<br",
      "Text"    : "Описание</td>(.*?)<script"
    }
  },
  /////////////////////////////////////////////////////////////////////////////
  // Неиспользуемые объекты для примера с устаревшими недействующими правилами
  "7tor.org": {
    "Conditions": [
      {
        "ConditionLink": "/viewtopic.php\\?t=\\d+",
        "Rule": "Torrent"
      },
      {
        "ConditionText": "(feed-icon-forum)",
        "Block": "(<a[^>]+forumtitle.*?</a>)",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=\"(.*?)\""
      },
      {
        "ConditionText": "(class=[^>]+topics)",
        "Cut": "(<ul[^>]+class=\"[^\"]*?topics.*?</ul>)",
        "Block": "<li(.*?)</li>",
        "Title": "(<a[^>]+viewtopic.php.*?</a>)",
        "Link": "<a[^>]+href=\"([^\"]*?viewtopic.php.*?)\""
      }
    ],
    "Torrent": {
      "Subs": "Субтитры.*?(:.*?)<",
      "Url": "(/download/file.php\\?id=\\d+)",
      "Method": "GET",
      "Seeders": "class=\"[^\"]*?seed.*?(\\d+)",
      "Leechers": "class=\"[^\"]*?leech.*?(\\d+)",
      "Date": "Зарегистрирован.*?>(.*?)<",
      "Size": "Размер:.*?<td>(.*?)</td>",
      "Image": "prettyPhotoPosters.*?<img[^>]+src=\"(.*?)\"",
      "Year": "Год выпуска.*?(\\d{4})",
      "Duration": "(?:Время звучания|Продолжительность)(.*?)<br",
      "Genre": "Жанр.*?:(.*?)<",
      "Text": "(<div[^>]+content.*?)<div[^>]+sp-wrap"
    }
  },
  // --------------------------------------------------------------------------
  "bigtorrent.org": {
    "Conditions": [
      {
        "ConditionLink": "(\\.html)$",
        "Rule": "Torrent"
      },
      {
        "ConditionText": "class=\"post\"",
        "Block": "class=\"post\"[^>]+(rel=\".*?)post-footer",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=['\"]([^'\"]+)",
        "NextPage": "<a[^>]+href=\"([^\"]+)\"[^>]+title=\"next page\""
      }
    ],
    "Torrent": {
      "Url": "(/engine/download.php\\?id=\\d+)",
      "Seeders": "class[^>]+seed.*?(\\d+)",
      "Leechers": "class[^>]+leech.*?(\\d+)",
      "Date": "Торрент</td>.*?(<td.*?</td>)",
      "Size": "Размер.*?>(.*?)<",
      "Image": "<img[^>]+src=['\"](/uploads.*?)['\"]",
      "Year": "Год выпуска.*?(\\d{4})",
      "Duration": "(?:Время звучания|Продолжительность).*?>(.*?)<br",
      "Genre": "Категория.*?>(.*?)<",
      "Quality": "Качество:.*?>(.*?)<",
      "Downloads": ".torrent скачан.*?(<td.*?</td>)",
      "Text": "(<div[^>]+post-content.*?)<table"
    }
  },
  // --------------------------------------------------------------------------
  "riperam.org": {
    "Conditions": [
      {
        "ConditionLink": "(page\\d+\\.html)$",
        "Block": "class[^>]+row(.*?)</li>",
        "Title": "(<a[^>]+class=\"(forum|topic)title\".*?</a>)",
        "Link": "<a[^>]+href=\"([^\"]+)\"[^>]+class=\"(forum|topic)title\"",
        "NextPage": ".*<a[^>]+href=\"([^\"]+)\"[^>]+>След.</a>"
      },
      {
        "ConditionLink": "(\\.html)$",
        "Rule": "Torrent"
      },
      {
        "ConditionText": "<ul[^>]+(forums|topics)",
        "Block": "class[^>]+row(.*?)</li>",
        "Title": "(<a[^>]+class=\"(forum|topic)title\".*?</a>)",
        "Link": "<a[^>]+href=\"([^\"]+)\"[^>]+class=\"(forum|topic)title\"",
        "NextPage": ".*<a[^>]+href=\"([^\"]+)\"[^>]+>След.</a>"
      }
    ],
    "Torrent": {
      "Url": "(/download/file.php\\?id=\\d+)",
      "Seeders": "class[^>]+seed.*?(\\d+)",
      "Leechers": "class[^>]+leech.*?(\\d+)",
      "Date": "Торрент</td>.*?(<td.*?</td>)",
      "Size": "Размер.*?>(.*?)<",
      "Image": "poster.*?<img[^>]+src=['\"](http.*?)['\"]",
      "Year": "Год выпуска.*?(\\d{4})",
      "Duration": "(?:Время звучания|Продолжительность).*?>(.*?)<br",
      "Genre": "Жанр.*?>(.*?)<",
      "Quality": "Качество:.*?>(.*?)<",
      "Downloads": ".torrent скачан.*?(<td.*?</td>)",
      "Text": "(<div[^>]+content.*?)</td"
    }
  },
  // --------------------------------------------------------------------------
  "tfile.co": {
    "Conditions": [
      {
        "ConditionLink": "viewtopic.php\\?t=\\d+",
        "Rule": "Torrent"
      },
      {
        "ConditionLink": "/forum/\\?c=\\d+",
        "Block": "(<a[^>]+viewforum.php\\?f=\\d+.*?</a>)",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=\"(/.*?)\""
      },
      {
        "ConditionText": "<tr[^>]+id=\"t\\d+\"",
        "Block": "class=\"t\"(.*?)</tr>",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=\"(.*?)\"",
        "NextPage": "pagination.*?<a[^>]+href=\"(.[^\"]+)\""
      }
    ],
    "Torrent": {
      "Url": [
        "](download.php\\?id=\\d+.*?)["
      ],
      "Error": "class=\"copyrasted\">(.*?)<",
      "Seeders": "(<span[^>]+seeders.*?</span>)",
      "Leechers": "(<span[^>]+leechers.*?</span>)",
      "Date": "title=\"Ссылка\">(.*?)</a>",
      "Size": "Размер содержимого.*?(<span.*?</span>)",
      "Image": "class=\"pT\".*?<img[^>]+src=[\"'](http.*?)[\"']",
      "Year": "Год выпуска.*?(\\d{4})",
      "Duration": "Продолжительность.*?>(.*?)<",
      "Genre": "Жанр:?(.*?)<br",
      "Text": "(<div[^>]+class=\"pT\".*?)<[^>]+clear=\"all\""
    }
  },
  // --------------------------------------------------------------------------
  "tfile-music.org": {
    "Conditions": [
      {
        "ConditionLink": "viewtopic.php\\?t=\\d+",
        "Rule": "Torrent"
      },
      {
        "ConditionLink": "/forum/\\?c=\\d+",
        "Block": "(<a[^>]+viewforum.php\\?f=\\d+.*?</a>)",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=\"(/.*?)\""
      },
      {
        "ConditionText": "<tr[^>]+id=\"t\\d+\"",
        "Block": "class=\"t\"(.*?)</tr>",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=\"(.*?)\"",
        "NextPage": "pagination.*?<a[^>]+href=\"(.[^\"]+)\""
      }
    ],
    "Torrent": {
      "Url": [
        "](download.php\\?id=\\d+.*?)["
      ],
      "Error": "class=\"copyrasted\">(.*?)<",
      "Seeders": "(<span[^>]+seeders.*?</span>)",
      "Leechers": "(<span[^>]+leechers.*?</span>)",
      "Date": "title=\"Ссылка\">(.*?)</a>",
      "Size": "Размер содержимого.*?(<span.*?</span>)",
      "Image": "class=\"pT\".*?<img[^>]+src=[\"'](http.*?)[\"']",
      "Year": "Год выпуска.*?(\\d{4})",
      "Duration": "Продолжительность.*?>(.*?)<",
      "Genre": "Жанр:?(.*?)<br",
      "Text": "(<div[^>]+class=\"pT\".*?)<[^>]+clear=\"all\""
    }
  },
  // --------------------------------------------------------------------------
  "www.torrentino.net": {
    "Conditions": [
      {
        "ConditionLink": "/torrent/",
        "Rule": "Torrent"
      },
      {
        "ConditionText": "<tr[^>]+class=\"item",
        "Block": "(<tr[^>]+class=\"item.*?</tr>)",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=\"(.*?)\"",
        "PageInfo": {
          "Pages": 4,
          "PagesParam": "?&page=<PN>"
        },
        "NextPage": ".*<a[^>]+href=\"([^\"]+page=.*?)\">След"
      }
    ],
    "Torrent": {
      "Url": "([^\"]+/torrent/\\d+/download.*?)",
      "Error": "class=\"copyrasted\">(.*?)<",
      "Seeders": "Сиды.*?>(.*?)<",
      "Leechers": "Личи.*?>(.*?)<",
      "Date": "(<span[^>]+pdate.*?</span>)",
      "Size": "<span>Скачать[^<>]+?(\\d.*?)<",
      "Image": "class=\"info\".*?<img[^>]+src=[\"'](.*?)[\"']",
      "Year": "Год:.*?(\\d{4})",
      "Duration": "Продолжительность:?(.*?)<br",
      "Genre": "Жанр:(.*?)<br",
      "Text": "Название:(.*?)<div[^>]+share",
      "Author": "Исполнитель:(.*?)<br",
      "Album": "Название:(.*?)<br"
    }
  },
  // --------------------------------------------------------------------------
  "nnportal.org": {
    "Login": {
      "Url": "http://nnportal.org/portal/ucp.php?mode=login",
      "PostData": "username=<LOGIN>&password=<PASSW>&login=%D0%92%D1%85%D0%BE%D0%B4",
      "Success": "Moved Temporarily",
      "NoRedirect": 1
    },
    "Conditions": [
      {
        "ConditionText": "<a[^>]+cat_title[^>]+href=\"http",
        "Block": "(<a[^>]+cat_title[^>]+href=\"http.*?</a>)",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=\"(.*?)\""
      },
      {
        "ConditionText": "<a[^>]+forumlink",
        "Block": "(<a[^>]+forumlink.*?</a>)",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=\"(.*?)\""
      },
      {
        "ConditionText": "<a[^>]+topictitle",
        "Block": "(<a[^>]+topictitle.*?</a>)",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=\"(.*?)\"",
        "NextPage": ".*<a[^>]+href=\"([^\"]+/page.*?)\">След"
      },
      {
        "ConditionText": "torrent.php\\?mode=download",
        "Rule": "Torrent"
      }
    ],
    "Torrent": {
      "Url": "(/torrent.php\\?mode=download&.*?)\"",
      "Error": "<td class=\"c1 border_lr p6 gen\">(.*?)</td>",
      "Seeders": "Раздают.*?>(.*?)</span",
      "Leechers": "Качают.*?>(.*?)</span",
      "Date": "Зарегистрирован.*?>(.*?)<",
      "Size": "Размер:.*?>(.*?)<",
      "Image": "postbody.*?<img[^>]+src=[\"'](.*?)[\"']",
      "Year": "Год выпуска:.*?(\\d{4})",
      "Duration": "Продолжительность:?(<.*?)<br",
      "Genre": ">Жанр:?(<.*?)<br",
      "Text": "(<div[^>]+postbody.*?)<div[^>]+(spoiler|clear)"
    }
  },
  // --------------------------------------------------------------------------
  "peerates.ws": {
    "Login": {
      "Url": "http://peerates.ws/login.php?do=login",
      "PostData": "do=login&s=&vb_login_username=<LOGIN>&vb_login_password=<PASSW>",
      "Success": "Set-Cookie",
      "NoRedirect": 1
    },
    "Conditions": [
      {
        "ConditionLink": "showthread.php\\?t=\\d+",
        "Rule": "Torrent"
      },
      {
        "ConditionText": "statusicon/(forum|thread)",
        "Block": "((statusicon/forum|threadtitle|vbpostrow_).*?<a[^>]+(forumdisplay|showthread.php\\?[tp]=).*?</a>)",
        "Title": "(<a[^>]+(forumdisplay|showthread.php\\?[tp]=).*?</a>)",
        "Link": "<a[^>]+href=\"((forumdisplay|showthread.php\\?[tp]=).*?)\"",
        "NextPage": "<a[^>]+href=\"([^\"]+page=\\d+)\" title=\"Следующая"
      }
    ],
    "Torrent": {
      "Url": "href=\"(attachment.php.*?)\"",
      "Seeders": "attachment.php.*?Сиды.*?>(.*?)(,|<)",
      "Leechers": "attachment.php.*?Личеры.*?>(.*?)(,|<)",
      "Size": "attachment.php.*?Размер:.*?>(.*?)(,|<)",
      "Image": "post_message_.*?<img[^>]+src=[\"'](.*?)[\"']",
      "Year": "Год выпуска.*?(\\d{4})",
      "Duration": "Продолжительность?(.*?)<br",
      "Genre": "Жанр.*?:(.*?)<br",
      "Text": "(<div[^>]+post_message_.*?)(<[^>]+spoiler|</div>)"
    }
  },
  // --------------------------------------------------------------------------
  "torrents.omsk.ru": {
    "Login": {
      "Url": "http://torrents.omsk.ru/login.php?login_username=<LOGIN>&login_password=<PASSW>&login=%C2%F5%EE%E4",
      "Method": "POST",
      "NoRedirect": 1,
      "NoCache": 1,
      "Success": "Set-Cookie"
    },
    "Conditions": [
      {
        "ConditionLink": "viewtopic.php\\?t=\\d+",
        "Rule": "Torrent"
      },
      {
        "ConditionText": "(forumlink|<a[^>]+torTopic)",
        "Block": "(<h\\d[^>]+forumlink.*?</h\\d>|<a[^>]+torTopic.*?</a>)",
        "Title": "(<a.*</a>)",
        "Link": "<a[^>]+href=\"(.*?)\"",
        "NextPage": ".*<a[^>]+href=\"([^\"]+)\">След.</a>"
      }
    ],
    "Torrent": {
      "Url": "(download.php\\?id=\\d+)",
      "Error": "(<p[^>]+attach_link.*?</p>)",
      "Seeders": "Seeders:.*?(\\d+)",
      "Leechers": "Качают:.*?(\\d+)",
      "Date": "Зарегистрирован.*?>(.*?)<",
      "Size": "Размер:.*?>(.*?)<",
      "Image": "<img[^>]+src=\"([^\"]+)\"[^>]+postImgAligned",
      "Year": "Год выпуска.*?(\\d{4})",
      "Duration": "Продолжительность.*?>(.*?)<br",
      "Genre": "Жанр.*?:(.*?)<",
      "Subs": "Субтитры.*?(:.*?)<br",
      "Text": "(<div[^>]+post_body.*?)<div[^>]+(spoiler-body|class=\"clear\")"
    }
  },
  // --------------------------------------------------------------------------
  "hdpicture.ru": {
    "Conditions": [
      {
        "ConditionLink": "(\\.html)$",
        "Rule": "Torrent"
      },
      {
        "ConditionText": "class=\"news\"",
        "Block": "class=\"news\"(.*?)class=\"n-views\"",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=\"(.*?)\"",
        "NextPage": ".*<a[^>]+href=\"([^\"]+)[^>]+>Далее</a>"
      }
    ],
    "Torrent": {
      "Url": "(/engine/download.php\\?id=\\d+)",
      "Seeders": null,
      "Leechers": null,
      "Date": "class=\"n-date\">(.*?)<",
      "Size": "Размер:.*?>(.*?)<",
      "Image": "sstory.*?<img[^>]+src=['\"](http.*?)['\"]",
      "Year": "Год выхода.*?(\\d{4})",
      "Duration": "(?:Время звучания|Продолжительность).*?>(.*?)<br",
      "Genre": "Жанр:.*?>(.*?)<",
      "Quality": "Качество:.*?>(.*?)<",
      "Downloads": "class=\"li_download\">(.*?)</",
      "Text": "(<div[^>]+sstory.*?)<div id=\""
    }
  },
  // --------------------------------------------------------------------------
  "kinohd.net": {
    "Conditions": [
      {
        "ConditionLink": "(\\.html)$",
        "Rule": "Torrent"
      },
      {
        "ConditionText": "<article",
        "Block": "(<article.*?</article>)",
        "Title": "(<h\\d.*?</h\\d>)",
        "Link": ".*<a[^>]+href=\"(.*?)\"",
        "NextPage": ".*<a[^>]+href=\"([^\"]+)[^>]+>Далее</a>"
      }
    ],
    "Torrent": {
      "Url": "(/engine/torrent.php\\?nid=\\d+&id=\\d+)",
      "Seeders": null,
      "Leechers": null,
      "Date": "Дата публикации:(.*?)<",
      "Size": "Размер:(.*?)<",
      "Image": "ftext.*?<img[^>]+src=['\"](.*?)['\"]",
      "Year": "ftext.*?Год выпуска.*?(\\d{4})",
      "Duration": "ftext.*?(?:Время звучания|Продолжительность):(.*?)<br",
      "Genre": "ftext.*?Жанр:(.*?)<",
      "Quality": "ftext.*?Качество:(.*?)<",
      "Downloads": "скачиваний:(.*?)</span>",
      "Text": "(<div[^>]+fullstory.*?)</table>"
    }
  },
  // --------------------------------------------------------------------------
  "rgfootball.net": {
    "Login": {
      "Url": "http://rgfootball.net/login.php?login_username=<LOGIN>&login_password=<PASSW>&login=%D0%92%D1%85%D0%BE%D0%B4",
      "Method": "POST",
      "NoRedirect": 1,
      "NoCache": 1,
      "Success": "Moved Temporarily"
    },
    "Conditions": [
      {
        "ConditionLink": "viewtopic.php\\?t=\\d+",
        "Rule": "Torrent"
      },
      {
        "ConditionText": "(forumlink|<a[^>]+id=\"tt-)",
        "Block": "(<h\\d[^>]+forumlink.*?</h\\d>|<a[^>]+id=\"tt-.*?</a>)",
        "Title": "(<a.*</a>)",
        "Link": "<a[^>]+href=\"(.*?)\"",
        "NextPage": ".*<a[^>]+href=\"([^\"]+)\">След.</a>"
      }
    ],
    "Torrent": {
      "Subs": "Субтитры.*?(:.*?)<",
      "Url": "(download.php\\?id=\\d+)",
      "Method": "POST",
      "Error": "(<th[^>]+id=\"opener\".*?)<div",
      "Seeders": "class=\"seed\">.*?(\\d+)",
      "Leechers": "class=\"leech\">.*?(\\d+)",
      "Date": "Зарегистрирован:.*?>(.*?)</",
      "Size": "Размер:.*?>(.*?)</",
      "Image": "postImgAligned[^>]+title=\"(http[^\"]+(jpg|png))\"",
      "Year": "Год выпуска.*?(\\d{4})",
      "Duration": "(?:Время звучания|Продолжительность).*?(\\d{2}:\\d{2}:\\d{2}|\\d{2}:\\d{2})",
      "Genre": "Вид спорта.*?:(.*?)</span>",
      "Text": "(<div[^>]+post_body.*?)<div[^>]+sp-wrap"
    }
  },
  // --------------------------------------------------------------------------
  "nnm-club.name": {
    "Login": {
      "Load": "http://nnm-club.name/forum/login.php",
      "CODE": "name=\"code\"[^>]+value=\"(.*?)\"",
      "Url": "http://nnm-club.name/forum/login.php?username=<LOGIN>&password=<PASSW>&autologin=on&redirect=index.php&code=<CODE>&login=%C2%F5%EE%E4",
      "Method": "POST",
      "NoRedirect": 1,
      "NoCache": 1,
      "Success": "302 Moved"
    },
    "Conditions": [
      {
        "ConditionLink": "\\?t=\\d+",
        "Rule": "Torrent"
      },
      {
        "ConditionLink": "nnm-club.name$",
        "Cut": "<b>Разделы</b>(.*?)</table>",
        "Block": "(<a.*?</a>)",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=['\"]([^'\"]+)"
      },
      {
        "ConditionLink": "\\?c=\\d+",
        "Block": "<h2(.*?)</h2>",
        "Title": "(<a.*?</a>)",
        "Link": "<a[^>]+href=['\"]([^'\"]+)",
        "NextPage": ".*<a[^>]+href=\"([^\"]+)\">След.</a>"
      }
    ],
    "Torrent": {
      "Url": "(download.php\\?id=\\d+)",
      "Seeders": "class[^>]+seed.*?(\\d+)",
      "Leechers": "class[^>]+leech.*?(\\d+)",
      "Date": "Зарегистрирован:.*?</td>.*?(<td.*?</td>)",
      "Size": "Размер:.*?</td>.*?(<td.*?</td>)",
      "Image": "postImg.*?title=['\"](http.*?)['\"]",
      "Year": "postbody.*?\\((\\d{4})\\)",
      "Duration": "(?:Время звучания|Продолжительность).*?>(.*?)<br",
      "Genre": "Жанр.*?span>(.*?)<",
      "Quality": "Качество.*?span>(.*?)<",
      "Text": "(<div[^>]+postbody.*?)</tr>"
    }
  }
}