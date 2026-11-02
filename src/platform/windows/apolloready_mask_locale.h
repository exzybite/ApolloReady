#pragma once
#include <string>
#include <string_view>

namespace apolloready {
  struct mask_translation {
    std::string_view locale;
    const wchar_t *loading;
    const wchar_t *loading_detail;
    const wchar_t *closing;
    const wchar_t *closing_detail;
    const wchar_t *background;
    const wchar_t *background_detail;
  };

  inline constexpr mask_translation mask_translations[] = {
    {"en", L"Loading game", L"Your game will appear automatically.", L"Closing game", L"Ending session...", L"Game in background", L"Return to the game to resume."},
    {"fr", L"Chargement du jeu", L"Le jeu apparaîtra automatiquement.", L"Fermeture du jeu", L"Fin de la session...", L"Jeu en arrière-plan", L"Revenez au jeu pour reprendre."},
    {"de", L"Spiel wird geladen", L"Das Spiel wird automatisch angezeigt.", L"Spiel wird beendet", L"Sitzung wird beendet...", L"Spiel im Hintergrund", L"Kehre zum Spiel zurück, um fortzufahren."},
    {"es", L"Cargando el juego", L"El juego aparecerá automáticamente.", L"Cerrando el juego", L"Finalizando la sesión...", L"Juego en segundo plano", L"Vuelve al juego para continuar."},
    {"it", L"Caricamento del gioco", L"Il gioco apparirà automaticamente.", L"Chiusura del gioco", L"Chiusura della sessione...", L"Gioco in background", L"Torna al gioco per continuare."},
    {"pt", L"A carregar o jogo", L"O jogo aparecerá automaticamente.", L"A fechar o jogo", L"A terminar a sessão...", L"Jogo em segundo plano", L"Volta ao jogo para continuar."},
    {"pt-br", L"Carregando o jogo", L"O jogo aparecerá automaticamente.", L"Fechando o jogo", L"Encerrando a sessão...", L"Jogo em segundo plano", L"Volte ao jogo para continuar."},
    {"sv", L"Laddar spelet", L"Spelet visas automatiskt.", L"Avslutar spelet", L"Avslutar sessionen...", L"Spelet körs i bakgrunden", L"Återgå till spelet för att fortsätta."},
    {"pl", L"Ładowanie gry", L"Gra pojawi się automatycznie.", L"Zamykanie gry", L"Kończenie sesji...", L"Gra w tle", L"Wróć do gry, aby kontynuować."},
    {"cs", L"Načítání hry", L"Hra se zobrazí automaticky.", L"Ukončování hry", L"Ukončování relace...", L"Hra na pozadí", L"Vraťte se do hry a pokračujte."},
    {"bg", L"Зареждане на играта", L"Играта ще се покаже автоматично.", L"Затваряне на играта", L"Приключване на сесията...", L"Играта е на заден план", L"Върнете се в играта, за да продължите."},
    {"ru", L"Загрузка игры", L"Игра появится автоматически.", L"Закрытие игры", L"Завершение сеанса...", L"Игра в фоновом режиме", L"Вернитесь в игру, чтобы продолжить."},
    {"uk", L"Завантаження гри", L"Гра з’явиться автоматично.", L"Закриття гри", L"Завершення сеансу...", L"Гра у фоновому режимі", L"Поверніться до гри, щоб продовжити."},
    {"tr", L"Oyun yükleniyor", L"Oyun otomatik olarak görüntülenecek.", L"Oyun kapatılıyor", L"Oturum sonlandırılıyor...", L"Oyun arka planda", L"Devam etmek için oyuna dönün."},
    {"ja", L"ゲームを読み込み中", L"ゲームは自動的に表示されます。", L"ゲームを終了中", L"セッションを終了しています…", L"ゲームはバックグラウンドです", L"ゲームに戻ると再開します。"},
    {"ko", L"게임 불러오는 중", L"게임이 자동으로 표시됩니다.", L"게임 종료 중", L"세션을 종료하고 있습니다...", L"게임이 백그라운드에 있습니다", L"계속하려면 게임으로 돌아가세요."},
    {"zh", L"正在加载游戏", L"游戏画面将自动显示。", L"正在关闭游戏", L"正在结束会话…", L"游戏正在后台运行", L"返回游戏以继续。"},
    {"zh-tw", L"正在載入遊戲", L"遊戲畫面將自動顯示。", L"正在關閉遊戲", L"正在結束工作階段…", L"遊戲正在背景執行", L"返回遊戲以繼續。"},
  };

  inline const mask_translation &mask_text(std::string_view locale) {
    std::string code(locale);
    for (auto &ch : code) {
      if (ch == '_') ch = '-';
      else if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
    }
    code.resize(code.find_first_of(".@") == std::string::npos ? code.size() : code.find_first_of(".@"));
    // Accept regional aliases while keeping traditional Chinese distinct.
    if (code == "zh-hk" || code == "zh-mo" || code.starts_with("zh-hant")) code = "zh-tw";
    for (const auto &translation : mask_translations) {
      if (code == translation.locale) return translation;
    }
    const auto language = std::string_view(code).substr(0, code.find('-'));
    for (const auto &translation : mask_translations) {
      if (language == translation.locale) return translation;
    }
    return mask_translations[0];
  }
}
