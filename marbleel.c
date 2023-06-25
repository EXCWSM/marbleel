/*
 * marbleel.c
 *
 * 2013,2019,2023 Copyright(c) EXCWSM  All rights reserved.
 */
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>

/* ****************************************************************** */
/*   Constants                                                        */
/* ****************************************************************** */

/* ウィンドウクラス名 */
#define MAINWIN_CLASSNAME      "MARBLEEL"

/* トレイアイコン 識別番号 */
#define MAINICON_ID            (1)

/* イベント番号 */
#define WM_MAINICON            (WM_APP + 0x8001) /* トレイ */
#define WM_ALTBUTTON           (WM_APP + 0x8110) /* 読み替え */
#define WM_EMUCLICK            (WM_APP + 0x8111) /* ホイールボタンのクリック */
#define WM_EMUWHEEL            (WM_APP + 0x8112) /* ホイール発行 */
#define WM_COMBODOWN           (WM_APP + 0x8121) /* 同時押しによるBUTTONDOWN発行 */
#define WM_COMBOUP             (WM_APP + 0x8122) /* 同時押しによるBUTTONUP発行 */
#define WM_COMBOBTN1DOWN       (WM_APP + 0x8123) /* 同時押し用ボタン1のBUTTONDOWN */
#define WM_COMBOBTN2DOWN       (WM_APP + 0x8124) /* 同時押し用ボタン2のBUTTONDOWN */
#define WM_COMBOBTN1UP         (WM_APP + 0x8125) /* 同時押し用ボタン1のBUTTONUP */
#define WM_COMBOBTN2UP         (WM_APP + 0x8126) /* 同時押し用ボタン2のBUTTONUP */

/* 実行状態 */
#define ES_READY               (0)             /* READY */
#define ES_PRESSED             (1)             /* エミュレーションボタンを押している */
#define ES_WHEELING            (2)             /* ホイールエミュレーション実行中 */
#define ES_WHEELED             (3)             /* ホイールエミュレーションを終えた */

/* 設定 */
#define CMDRET_QUIT            (0xFFFFFFFF)    /* 終了 */

/* (Vista以降) 横ホイール */
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL         (0x020E)
#endif
#ifndef MOUSEEVENTF_HWHEEL
#define MOUSEEVENTF_HWHEEL     (0x01000)
#endif

/* SendInput()等によるものか */
#ifndef LLMHF_LOWER_IL_INJECTED
#define LLMHF_LOWER_IL_INJECTED (0x0002)
#endif

/* アプリケーション表示名 */
const TCHAR *strAppTitle = _T("Marbleel 1.3");

/* ボタンの表示名 */
const TCHAR *STR_NONE        = _T("none");
const TCHAR *STR_LBUTTON     = _T("1(L)");
const TCHAR *STR_RBUTTON     = _T("2(R)");
const TCHAR *STR_MBUTTON     = _T("3(M)");
const TCHAR *STR_XBUTTON1    = _T("4(X1)");
const TCHAR *STR_XBUTTON2    = _T("5(X2)");

/* コマンドラインオプション */
const TCHAR *CMDOPT_BUTTON   = _T("/b");     /* ホイール駆動ボタン */
const TCHAR *CMDOPT_VDELTA   = _T("/vd");    /* 縦 WHEEL_DELTA */
const TCHAR *CMDOPT_HDELTA   = _T("/hd");    /* 横 WHEEL_DELTA */
const TCHAR *CMDOPT_VSCOUNT  = _T("/vc");    /* 縦 駆動判定回数 */
const TCHAR *CMDOPT_HSCOUNT  = _T("/hc");    /* 横 駆動判定回数 */
const TCHAR *CMDOPT_VSENSE   = _T("/vs");    /* 縦 駆動判定ポイント */
const TCHAR *CMDOPT_HSENSE   = _T("/hs");    /* 横 駆動判定ポイント */
const TCHAR *CMDOPT_ALTER    = _T("/a");     /* ボタン読み替え */
const TCHAR *CMDOPT_COMBO    = _T("/c");     /* ボタン同時押し */
const TCHAR *CMDOPT_CTIME    = _T("/ct");    /* 同時押し判定時間ミリ秒 */
const TCHAR *CMDOPT_CLICK    = _T("/e");     /* ホイール駆動ボタンのクリック有効 */
const TCHAR *CMDOPT_HWHEEL   = _T("/h");     /* 横ホイール有効 */
const TCHAR *CMDOPT_REVERSE  = _T("/r");     /* ホイール反転 */
const TCHAR *CMDOPT_QUIT     = _T("/q");     /* 既に起動しているものを終了 */

/* ****************************************************************** */
/*   Structures                                                       */
/* ****************************************************************** */

/* ボタン読み替え定義 */
typedef struct tagEmuButton {
    DWORD src;                 /* 元 WM_*BUTTON_* */
    DWORD src_x;               /* 元 XBUTTONの番号 */
    DWORD dst;                 /* 先 WM_*BUTTON* */
    INPUT i;                   /* 先 INPUT */
    struct tagEmuButton *next; /* 次のEmuButtonまたはNULL */
} EmuButton;

/* ****************************************************************** */
/*   Variables                                                        */
/* ****************************************************************** */

/* ボタン読み替え定義のリスト */
EmuButton *emubuttons = NULL;

/* ホイールエミュレーション ボタン定義 */
DWORD emuwheel_button_wm_down = 0;
DWORD emuwheel_button_wm_up = 0;
DWORD emuwheel_button_wm_x = 0;

/* 同時押し設定 */
DWORD combo_button_time = 75;          /* 同時押し判定ミリ秒 */
DWORD combo_button1_wm_down = 0;
DWORD combo_button1_wm_up = 0;
DWORD combo_button2_wm_down = 0;
DWORD combo_button2_wm_up = 0;

/* 横ホイールあり */
BOOL emuwheel_horizontal_enabled = FALSE;

/* ホイール反転 */
BOOL emuwheel_reverse = FALSE;

/* 感度設定 */
int emuwheel_vsens = 1;
int emuwheel_vsens_count = 1;
int emuwheel_hsens = 1;
int emuwheel_hsens_count = 1;

/* 送信スレッドのID */
DWORD idSenderThread = 0;

/* 発動ボタン クリック発行用 */
BOOL  emuwheel_click_enabled = FALSE;  /* クリックを発行するかどうか */
INPUT emuwheel_button_mi_down;
INPUT emuwheel_button_mi_up;

/* ホイール命令発行用 */
INPUT emuwheel_mi_down;
INPUT emuwheel_mi_up;
INPUT emuwheel_mi_left;
INPUT emuwheel_mi_right;

/* 同時押し 発行用 */
INPUT combo_mi_down;                /* 同時押し発行用DOWN */
INPUT combo_mi_up;                  /* 同時押し発行用UP */
BOOL pressed_combo_buttons = 0;     /* 同時押し中 */
SHORT btnState = 0;                 /* ボタン1&2状況 */
BOOL combo_button1_down = FALSE;    /* ボタン1単独DOWN中 */
BOOL combo_button2_down = FALSE;    /* ボタン2単独DOWN中 */
BOOL combo_button1_up = FALSE;      /* ボタン1単独UP中 */
BOOL combo_button2_up = FALSE;      /* ボタン2単独UP中 */
INPUT combo_button1_mi_down;        /* ボタン1単独DOWN */
INPUT combo_button2_mi_down;        /* ボタン1単独DOWN */
INPUT combo_button1_mi_up;          /* ボタン1単独UP */
INPUT combo_button2_mi_up;          /* ボタン2単独UP */
HANDLE hEventComboButton1Up;        /* ボタン1のUPを通知 */
HANDLE hEventComboButton2Up;        /* ボタン2のUPを通知 */

/* ****************************************************************** */
/*   Functions                                                        */
/* ****************************************************************** */
void showErrorDialog(DWORD, DWORD);

const TCHAR * getMouseButtonStr(DWORD, DWORD);
const TCHAR * getMouseEventButtonStr(DWORD, DWORD);
void showAppVersionDialog(void);
void showConfigDialog(void);
void freeEmuButtons(void);
BOOL appendEmuButton(DWORD, DWORD, DWORD, DWORD, DWORD);
int parseCommandLine(LPTSTR);
LRESULT CALLBACK MAINWIN_WndProc(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK GLOBALHOOK_MouseProc(int, WPARAM, LPARAM);
DWORD WINAPI RECEIVER_ThreadProc(LPVOID);

DWORD WINAPI SENDER_ThreadProc(LPVOID);

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);

/* RtlZeroMemory() は WinAPI を用いる */
#undef RtlZeroMemory
EXTERN_C NTSYSAPI VOID NTAPI RtlZeroMemory(LPVOID UNALIGNED, SIZE_T);
/* RtlMoveMemory() は WinAPI を用いる */
#undef RtlMoveMemory
EXTERN_C NTSYSAPI VOID NTAPI RtlMoveMemory(LPVOID UNALIGNED, const LPVOID UNALIGNED, SIZE_T);

/* ****************************************************************** */
/*   Implementation                                                   */
/* ****************************************************************** */

/*
 * DEBUG_PRINT
 */
#ifdef DEBUG
#include <stdarg.h>
#define DEBUG_SPACE (0x01)
#define DEBUG_STR   (0x02)
#define DEBUG_INT   (0x03)
#define DEBUG_HEX   (0x04)
void DEBUG_PRINT(TCHAR *msg, ...)
{
    va_list vars;
    DWORD n;
    TCHAR s[256], tmp[32], *ps;

    lstrcpy(s, msg);

    va_start(vars, msg);
    do {
        n = va_arg(vars, DWORD);
        switch (n) {
        case DEBUG_SPACE:
            lstrcat(s, _T(" "));
            break;
        case DEBUG_STR:
            ps = va_arg(vars, TCHAR *);
            if (ps == NULL) {
                lstrcat(s, _T("(NULL)"));
            } else {
                lstrcat(s, ps);
            }
            break;
        case DEBUG_INT:
            wsprintf(tmp, _T("%d"), (long int)va_arg(vars, long int));
            lstrcat(s, tmp);
            break;
        case DEBUG_HEX:
            wsprintf(tmp, _T("0x%X"), (long int)va_arg(vars, long int));
            lstrcat(s, tmp);
            break;
        }
    } while (n != 0);
    va_end(vars);

    lstrcat(s, _T("\r\n"));
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), s, lstrlen(s) * sizeof(TCHAR), &n, NULL);
}

/* BCC5.5 に __VAR_ARGS__ はない */
#define DEBUG_PRINT0(_A_)              DEBUG_PRINT((_A_), 0);
#define DEBUG_PRINT1(_A_, _B_, _C_)    DEBUG_PRINT((_A_), DEBUG_SPACE, (_B_), (_C_), 0);

#else

#define DEBUG_PRINT0(_A_)              ;
#define DEBUG_PRINT1(_A_, _B_, _C_)    ;

#endif

/*
 * 共通エラーダイアログボックス
 */
void showErrorDialog(DWORD errcd, DWORD reason)
{
    TCHAR msg[64];

    wsprintf(msg, _T("CRITICAL ERROR\r\nCODE=0x%X, REASON=0x%X"), errcd, reason);
    MessageBox(NULL, msg, strAppTitle, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
}

/*
 * マウスボタン文字列を取得 WM_*BUTTONDOWN イベント番号系
 */
const TCHAR * getMouseButtonStr(DWORD button, DWORD xbutton)
{
    switch (button) {
    case WM_LBUTTONDOWN: return STR_LBUTTON;
    case WM_RBUTTONDOWN: return STR_RBUTTON;
    case WM_MBUTTONDOWN: return STR_MBUTTON;
    case WM_XBUTTONDOWN:
        switch (xbutton) {
        case XBUTTON1: return STR_XBUTTON1;
        case XBUTTON2: return STR_XBUTTON2;
        }
    }
    return STR_NONE;
}

/*
 * マウスボタン文字列を取得 INPUT.m系
 */
const TCHAR * getMouseEventButtonStr(DWORD flags, DWORD data)
{
    if ((flags & MOUSEEVENTF_LEFTDOWN) == MOUSEEVENTF_LEFTDOWN) {
        return STR_LBUTTON;
    }
    if ((flags & MOUSEEVENTF_RIGHTDOWN) == MOUSEEVENTF_RIGHTDOWN) {
        return STR_RBUTTON;
    }
    if ((flags & MOUSEEVENTF_MIDDLEDOWN) == MOUSEEVENTF_MIDDLEDOWN) {
        return STR_MBUTTON;
    }
    if ((flags & MOUSEEVENTF_XDOWN) == MOUSEEVENTF_XDOWN) {
        switch (data) {
        case XBUTTON1: return STR_XBUTTON1;
        case XBUTTON2: return STR_XBUTTON2;
        }
    }
    return STR_NONE;
}

/*
 * 設定内容表示
 */
void showConfigDialog(void)
{
    TCHAR msg[256];
    TCHAR tmp[32];
    EmuButton *eb;

    lstrcpy(msg, _T("Command line options:"));

    if (emuwheel_button_wm_down != 0) {
        wsprintf(tmp, _T("\r\n%s %s")
                 , CMDOPT_BUTTON
                 , getMouseButtonStr(emuwheel_button_wm_down, emuwheel_button_wm_x)
            );
        lstrcat(msg, tmp);
        if (emuwheel_click_enabled) {
            lstrcat(msg, _T("\r\n"));
            lstrcat(msg, CMDOPT_CLICK);
        }
        if (emuwheel_horizontal_enabled) {
            lstrcat(msg, _T("\r\n"));
            lstrcat(msg, CMDOPT_HWHEEL);
        }
        if (emuwheel_reverse) {
            lstrcat(msg, _T("\r\n"));
            lstrcat(msg, CMDOPT_REVERSE);
        }

        wsprintf(tmp, _T("\r\n%s %d %s %d %s %d %s %d")
                 , CMDOPT_VSENSE, emuwheel_vsens
                 , CMDOPT_HSENSE, emuwheel_hsens
                 , CMDOPT_VSCOUNT, emuwheel_vsens_count
                 , CMDOPT_HSCOUNT, emuwheel_hsens_count);
        lstrcat(msg, tmp);

        wsprintf(tmp, _T("\r\n%s %u %s %u")
                 , CMDOPT_VDELTA, emuwheel_mi_up.mi.mouseData
                 , CMDOPT_HDELTA, emuwheel_mi_right.mi.mouseData);
        lstrcat(msg, tmp);
    }

    if (combo_button1_wm_down != 0) {
        wsprintf(tmp, _T("\r\n%s %s %s %s\r\n%s %u")
                 , CMDOPT_COMBO
                 , getMouseEventButtonStr(combo_button1_mi_down.mi.dwFlags, combo_button1_mi_down.mi.mouseData)
                 , getMouseEventButtonStr(combo_button2_mi_down.mi.dwFlags, combo_button2_mi_down.mi.mouseData)
                 , getMouseEventButtonStr(combo_mi_down.mi.dwFlags, combo_mi_down.mi.mouseData)
                 , CMDOPT_CTIME, combo_button_time
            );
        lstrcat(msg, tmp);
    }

    for (eb = emubuttons; eb != NULL; eb = (eb->next ? eb->next->next : eb->next)) {
        wsprintf(tmp, _T("\r\n%s %s %s")
                 , CMDOPT_ALTER
                 , getMouseButtonStr(eb->src, eb->src_x)
                 , getMouseEventButtonStr(eb->i.mi.dwFlags, eb->i.mi.mouseData)
            );
        lstrcat(msg, tmp);
        if (225 <= lstrlen(msg)) {
            break;
        }
    }

    if (msg[0] != _T('\0'))
        MessageBox(NULL, msg, strAppTitle, MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
}

void showAppVersionDialog(void)
{
    MessageBox(NULL
               , _T("MOUSE WHEEL EMULATOR\r\n\r\nwritten by EXCWSM.")
               , strAppTitle
               , MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
}

/*
 * EmuButton リスト構造体を破棄
 */
void freeEmuButtons(void)
{
    EmuButton *eb;
    HANDLE heap = GetProcessHeap();

    while (emubuttons != NULL) {
        eb = emubuttons->next;
        HeapFree(heap, 0, emubuttons);
        emubuttons = eb;
    }
}

/*
 * EmuButton 追加
 *
 * src     WM_BUTTON*
 * src_x   XBUTTON*
 * dst     WM_BUTTON*
 * dst_x   XBUTTON*
 * idst    MOUSEEVENTF_*
 */
BOOL appendEmuButton(DWORD src, DWORD src_x, DWORD dst, DWORD dst_x, DWORD idst)
{
    EmuButton *eb;
    EmuButton *eb_new;

    /* 同じ元側があれば宛側を置換して終了
     * なければリストの末尾まで行く */
    for (eb = emubuttons; eb != NULL && eb->next != NULL; eb = eb->next) {
        if ( (eb->src == src) && (eb->src_x == src_x) ) {
            eb->dst = dst;
            eb->i.mi.dwFlags = dst | MOUSEEVENTF_ABSOLUTE;
            eb->i.mi.mouseData = dst_x;
            return TRUE;
        }
    }

    eb_new = (EmuButton *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(EmuButton));
    if (eb_new == NULL) {
        return FALSE;
    }

    eb_new->src = src;
    eb_new->src_x = src_x;
    eb_new->dst = dst;
    eb_new->i.type = INPUT_MOUSE;
    eb_new->i.mi.dwFlags = idst | MOUSEEVENTF_ABSOLUTE;
    eb_new->i.mi.mouseData = dst_x;
    eb_new->next = NULL;

    if (eb == NULL) {
        emubuttons = eb_new;
    } else {
        eb->next = eb_new;
    }

    return TRUE;
}

/*
 * コマンドラインによる設定
 */
int parseCommandLine(LPTSTR cmdline)
{
    /* ボタン指令構築用テーブル */
    const DWORD MOUSEBUTTONS[] = {
        /*       WM_*DOWN,       WM_*UP,       mouseData, MOUSEEVENTF_*DOWN,      MOUSEEVENTF_*UP */
        /* 左 */ WM_LBUTTONDOWN, WM_LBUTTONUP, 0,         MOUSEEVENTF_LEFTDOWN,   MOUSEEVENTF_LEFTUP
        /* 右 */,WM_RBUTTONDOWN, WM_RBUTTONUP, 0,         MOUSEEVENTF_RIGHTDOWN,  MOUSEEVENTF_RIGHTUP
        /* 中 */,WM_MBUTTONDOWN, WM_MBUTTONUP, 0,         MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP
        /* X1 */,WM_XBUTTONDOWN, WM_XBUTTONUP, XBUTTON1,  MOUSEEVENTF_XDOWN,      MOUSEEVENTF_XUP
        /* X2 */,WM_XBUTTONDOWN, WM_XBUTTONUP, XBUTTON2,  MOUSEEVENTF_XDOWN,      MOUSEEVENTF_XUP
    };
#define MB_COUNT   5
#define MB_WM_DOWN 0
#define MB_WM_UP   1
#define MB_XBUTTON 2
#define MB_ME_DOWN 3
#define MB_ME_UP   4

    /* コマンドライン分析用 */
#define PM_BUTTON      (0x0001)
#define PM_VDELTA      (0x0002)
#define PM_HDELTA      (0x0003)
#define PM_VSENSE      (0x0004)
#define PM_VSENSE_CNT  (0x0005)
#define PM_HSENSE      (0x0006)
#define PM_HSENSE_CNT  (0x0007)
#define PM_ALTER_SRC   (0x0011)
#define PM_ALTER_DST   (0x0012)
#define PM_COMBO_1     (0x0021)
#define PM_COMBO_2     (0x0022)
#define PM_COMBO_3     (0x0023)
#define PM_COMBO_TIME  (0x0024)

#define PARAM_SIZE     (16)

    int i, cont;                               /* コマンドライン文字列の指標 */
    TCHAR param[PARAM_SIZE];                   /* パラメータ */
    int ip = 0;                                /* paramの指標 */
    int pmode = 0;                             /* 分析中のパラメータ区分 */
    int j;                                     /* 数値処理用指標 */
    int num;                                   /* パラメータから取った数値 */

    int emuwheelButton;                        /* ホイールするためのボタン番号 */
    int emuwheelVWD = WHEEL_DELTA;             /* WHEEL_DELTA 縦 */
    int emuwheelHWD = WHEEL_DELTA;             /* WHEEL_DELTA 横 */
    int button1, button2, button3;             /* 番号処理用 */

    if (cmdline == NULL)
        return 0x0001;

    ZeroMemory(param, sizeof(TCHAR) * PARAM_SIZE);

    for (i=0, cont=1; cont; i++) {

        /* 次のパラメータを切り出す */
        if (cmdline[i] == _T('\0')) {
            param[ip] = _T('\0');
            cont = 0;
        } else if (cmdline[i] == _T(' ')) {
            param[ip] = _T('\0');
        } else {
            param[ip++] = cmdline[i];
            if (ip < (PARAM_SIZE-1))
                continue;
            param[ip] = _T('\0');
        }
        ip = 0;
        if (param[0] == _T('\0')) {
            continue;
        }

        /* スイッチを判定 */
        if (pmode == 0) {
            if (param[0] != _T('/')) {
                ;;
            } else if (lstrcmp(param, CMDOPT_BUTTON) == 0) {
                pmode = PM_BUTTON;
            } else if (lstrcmp(param, CMDOPT_VDELTA) == 0) {
                pmode = PM_VDELTA;
            } else if (lstrcmp(param, CMDOPT_HDELTA) == 0) {
                pmode = PM_HDELTA;
            } else if (lstrcmp(param, CMDOPT_VSCOUNT) == 0) {
                pmode = PM_VSENSE_CNT;
            } else if (lstrcmp(param, CMDOPT_HSCOUNT) == 0) {
                pmode = PM_HSENSE_CNT;
            } else if (lstrcmp(param, CMDOPT_VSENSE) == 0) {
                pmode = PM_VSENSE;
            } else if (lstrcmp(param, CMDOPT_HSENSE) == 0) {
                pmode = PM_HSENSE;
            } else if (lstrcmp(param, CMDOPT_ALTER) == 0) {
                pmode = PM_ALTER_SRC;
            } else if (lstrcmp(param, CMDOPT_COMBO) == 0) {
                pmode = PM_COMBO_1;
            } else if (lstrcmp(param, CMDOPT_CTIME) == 0) {
                pmode = PM_COMBO_TIME;
            } else if (lstrcmp(param, CMDOPT_CLICK) == 0) {
                emuwheel_click_enabled = TRUE;
            } else if (lstrcmp(param, CMDOPT_HWHEEL) == 0) {
                emuwheel_horizontal_enabled = TRUE;
            } else if (lstrcmp(param, CMDOPT_REVERSE) == 0) {
                emuwheel_reverse = 1;
            } else if (lstrcmp(param, CMDOPT_QUIT) == 0) {
                return CMDRET_QUIT;
            }
            continue;
        }

        /* 数値を得る */
        num = 0;
        for (j=0; (j < 4) && (param[j] != _T('\0')); j++) {
            if (! ( (_T('0') <= param[j]) && (param[j] <= _T('9')) ) ) {
                break;
            }
            num = (num * 10) + (param[j] - _T('0'));
        }
        if (j == 0) {
            goto nextparam;
        }

        if (pmode == PM_BUTTON) {
            if (num < 1 || 5 < num) {
                goto nextparam;
            }

            button1 = MB_COUNT * (num - 1);
            emuwheel_button_wm_down = MOUSEBUTTONS[button1 + MB_WM_DOWN];
            emuwheel_button_wm_up   = MOUSEBUTTONS[button1 + MB_WM_UP];
            emuwheel_button_wm_x    = MOUSEBUTTONS[button1 + MB_XBUTTON];

            ZeroMemory(&emuwheel_button_mi_down, sizeof(INPUT));
            emuwheel_button_mi_down.type = INPUT_MOUSE;
            emuwheel_button_mi_down.mi.dwFlags = MOUSEBUTTONS[button1 + MB_ME_DOWN] | MOUSEEVENTF_ABSOLUTE;
            emuwheel_button_mi_down.mi.mouseData = MOUSEBUTTONS[button1 + MB_XBUTTON];

            ZeroMemory(&emuwheel_button_mi_up, sizeof(INPUT));
            emuwheel_button_mi_up.type = INPUT_MOUSE;
            emuwheel_button_mi_up.mi.dwFlags = MOUSEBUTTONS[button1 + MB_ME_UP] | MOUSEEVENTF_ABSOLUTE;
            emuwheel_button_mi_up.mi.mouseData = MOUSEBUTTONS[button1 + MB_XBUTTON];

        } else if (pmode == PM_VDELTA) {
            if (15 <= num && num <= (WHEEL_DELTA * 16)) {
                emuwheelVWD = num;
            }

        } else if (pmode == PM_HDELTA) {
            if (15 <= num && num <= (WHEEL_DELTA * 16)) {
                emuwheelHWD = num;
            }

        } else if (pmode == PM_VSENSE) {
            if (1 <= num && num <= 127) {
                emuwheel_vsens = num;
            }

        } else if (pmode == PM_VSENSE_CNT) {
            if (1 <= num && num <= 127) {
                emuwheel_vsens_count = num;
            }

        } else if (pmode == PM_HSENSE) {
            if (1 <= num && num <= 127) {
                emuwheel_hsens = num;
            }

        } else if (pmode == PM_HSENSE_CNT) {
            if (1 <= num && num <= 127) {
                emuwheel_hsens_count = num;
            }

        } else if (pmode == PM_ALTER_SRC) {
            if (1 <= num && num <= 5) {
                button1 = num;
                pmode = PM_ALTER_DST;
                continue;
            }

        } else if (pmode == PM_ALTER_DST) {
            if (num < 1 || 5 < num || num == button1) {
                goto nextparam;
            }

            button1 = MB_COUNT * (button1 - 1);
            button2 = MB_COUNT * (num - 1);

            if (! appendEmuButton(MOUSEBUTTONS[button1 + MB_WM_DOWN], MOUSEBUTTONS[button1 + MB_XBUTTON]
                                  , MOUSEBUTTONS[button2 + MB_WM_DOWN], MOUSEBUTTONS[button2 + MB_XBUTTON]
                                  , MOUSEBUTTONS[button2 + MB_ME_DOWN] )) {
                return 0x1001;
            }
            if (! appendEmuButton(MOUSEBUTTONS[button1 + MB_WM_UP], MOUSEBUTTONS[button1 + MB_XBUTTON]
                                  , MOUSEBUTTONS[button2 + MB_WM_UP], MOUSEBUTTONS[button2 + MB_XBUTTON]
                                  , MOUSEBUTTONS[button2 + MB_ME_UP])) {
                return 0x1002;
            }

        } else if (pmode == PM_COMBO_1) {
            if (1 <= num && num <= 5) {
                pmode = PM_COMBO_2;
                button1 = num;
                continue;
            }

        } else if (pmode == PM_COMBO_2) {
            if (1 <= num && num <= 5 && num != button1) {
                pmode = PM_COMBO_3;
                button2 = num;
                continue;
            }

        } else if (pmode == PM_COMBO_3) {
            if (num < 1 || 5 < num || button1 == num || button2 == num) {
                goto nextparam;
            }

            button1 = MB_COUNT * (button1 - 1);
            button2 = MB_COUNT * (button2 - 1);
            button3 = MB_COUNT * (num - 1);

            combo_button1_wm_down = MOUSEBUTTONS[button1 + MB_WM_DOWN];
            combo_button2_wm_down = MOUSEBUTTONS[button2 + MB_WM_DOWN];
            combo_button1_wm_up = MOUSEBUTTONS[button1 + MB_WM_UP];
            combo_button2_wm_up = MOUSEBUTTONS[button2 + MB_WM_UP];

            ZeroMemory(&combo_button1_mi_down, sizeof(INPUT));
            combo_button1_mi_down.type = INPUT_MOUSE;
            combo_button1_mi_down.mi.dwFlags = MOUSEBUTTONS[button1 + MB_ME_DOWN] | MOUSEEVENTF_ABSOLUTE;
            combo_button1_mi_down.mi.mouseData = MOUSEBUTTONS[button1 + MB_XBUTTON];

            ZeroMemory(&combo_button2_mi_down, sizeof(INPUT));
            combo_button2_mi_down.type = INPUT_MOUSE;
            combo_button2_mi_down.mi.dwFlags = MOUSEBUTTONS[button2 + MB_ME_DOWN] | MOUSEEVENTF_ABSOLUTE;
            combo_button2_mi_down.mi.mouseData = MOUSEBUTTONS[button2 + MB_XBUTTON];

            ZeroMemory(&combo_button1_mi_up, sizeof(INPUT));
            combo_button1_mi_up.type = INPUT_MOUSE;
            combo_button1_mi_up.mi.dwFlags = MOUSEBUTTONS[button1 + MB_ME_UP] | MOUSEEVENTF_ABSOLUTE;
            combo_button1_mi_up.mi.mouseData = MOUSEBUTTONS[button1 + MB_XBUTTON];

            ZeroMemory(&combo_button2_mi_up, sizeof(INPUT));
            combo_button2_mi_up.type = INPUT_MOUSE;
            combo_button2_mi_up.mi.dwFlags = MOUSEBUTTONS[button2 + MB_ME_UP] | MOUSEEVENTF_ABSOLUTE;
            combo_button2_mi_up.mi.mouseData = MOUSEBUTTONS[button2 + MB_XBUTTON];

            ZeroMemory(&combo_mi_down, sizeof(INPUT));
            combo_mi_down.type = INPUT_MOUSE;
            combo_mi_down.mi.dwFlags = MOUSEBUTTONS[button3 + MB_ME_DOWN] | MOUSEEVENTF_ABSOLUTE;
            combo_mi_down.mi.mouseData = MOUSEBUTTONS[button3 + MB_XBUTTON];

            ZeroMemory(&combo_mi_up, sizeof(INPUT));
            combo_mi_up.type = INPUT_MOUSE;
            combo_mi_up.mi.dwFlags = MOUSEBUTTONS[button3 + MB_ME_UP] | MOUSEEVENTF_ABSOLUTE;
            combo_mi_up.mi.mouseData = MOUSEBUTTONS[button3 + MB_XBUTTON];

        } else if (pmode == PM_COMBO_TIME) {
            if (30 <= num && num <= 500) { /* 高橋名人は31.25に相当する */
                combo_button_time = num;
            }
        }

    nextparam:
        pmode = 0;
    }

    /* ホイール 縦 */
    ZeroMemory(&emuwheel_mi_down, sizeof(INPUT));
    emuwheel_mi_down.type = INPUT_MOUSE;
    emuwheel_mi_down.mi.dwFlags = MOUSEEVENTF_WHEEL | MOUSEEVENTF_ABSOLUTE;
    emuwheel_mi_down.mi.mouseData = emuwheelVWD * (emuwheel_reverse ? 1 : -1);
    MoveMemory(&emuwheel_mi_up, &emuwheel_mi_down, sizeof(INPUT));
    emuwheel_mi_up.mi.mouseData = emuwheelVWD * (emuwheel_reverse ? -1 : 1);

    /* ホイール 横 */
    ZeroMemory(&emuwheel_mi_left, sizeof(INPUT));
    emuwheel_mi_left.type = INPUT_MOUSE;
    emuwheel_mi_left.mi.dwFlags = MOUSEEVENTF_HWHEEL | MOUSEEVENTF_ABSOLUTE;
    emuwheel_mi_left.mi.mouseData = emuwheelHWD * (emuwheel_reverse ? 1 : -1);
    MoveMemory(&emuwheel_mi_right, &emuwheel_mi_left, sizeof(INPUT));
    emuwheel_mi_right.mi.mouseData = emuwheelHWD * (emuwheel_reverse ? -1 : 1);

    return 0;
}

/*
 * メインウィンドウ
 */
LRESULT CALLBACK MAINWIN_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE hReceiverThread = NULL;
    static DWORD idReceiverThread = 0;
    static HANDLE hSenderThread = NULL;

    unsigned long MAXEL, ret, n;
    HANDLE hEventReady;

    switch (msg) {
    case WM_MAINICON:
        if (wParam != MAINICON_ID) {
            break;
        }
        switch (lParam) {
        case WM_LBUTTONDBLCLK:
            if ((GetKeyState(VK_SHIFT) & 0x80) != 0) {
                DestroyWindow(hWnd);
            } else if ((GetKeyState(VK_MENU) & 0x80) != 0) {
                showAppVersionDialog();
            } else if ((GetKeyState(VK_CONTROL) & 0x80) != 0) {
                showConfigDialog();
            } else if (MessageBox(NULL, _T("Exit?"), strAppTitle, MB_YESNO | MB_ICONWARNING | MB_SYSTEMMODAL) == IDYES) {
                DestroyWindow(hWnd);
            }
            break;

        case WM_MBUTTONDBLCLK:
            showAppVersionDialog();
            break;

        case WM_RBUTTONDBLCLK:
            showConfigDialog();
            break;
        }
        return 0;

    case WM_CLOSE:
    case WM_DESTROY:
        DEBUG_PRINT0(_T("DESTROY MAINWIN"));
        MAXEL = 0;

        if (hReceiverThread != NULL) {
            DEBUG_PRINT1(_T("PostThreadMessage(RECEIVER, WM_CLOSE)"), DEBUG_HEX, idReceiverThread);
            if (! PostThreadMessage(idReceiverThread, WM_CLOSE, (WPARAM)0, (LPARAM)0)) {
                showErrorDialog(0x01990001, GetLastError());
            } else if (WaitForSingleObject(hReceiverThread, 2500) != 0) {
                showErrorDialog(0x01990002, GetLastError());
            } else if (GetExitCodeThread(hReceiverThread, &ret)) {
                MAXEL = ret;
            }
            CloseHandle(hReceiverThread);
            hReceiverThread = NULL;
        }

        if (hSenderThread != NULL) {
            DEBUG_PRINT1(_T("PostThreadMessage(SENDER, WM_CLOSE)"), DEBUG_HEX, idSenderThread);
            if (! PostThreadMessage(idSenderThread, WM_CLOSE, (WPARAM)0, (LPARAM)0)) {
                showErrorDialog(0x01990003, GetLastError());
            } else if (WaitForSingleObject(hSenderThread, 2500) != 0) {
                showErrorDialog(0x01990004, GetLastError());
            } else if (GetExitCodeThread(hSenderThread, &ret)) {
                if (MAXEL < ret) {
                    MAXEL = ret;
                }
            }
            CloseHandle(hSenderThread);
            hSenderThread = NULL;
        }

        PostQuitMessage(MAXEL);
        return 0;

    case WM_CREATE:

        DEBUG_PRINT0(_T("CreateEvent()"));
        hEventReady = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hEventReady == NULL) {
            MAXEL = 0x0101; goto MAINWIN_WM_CREATE_EXCEPT;
        }
        DEBUG_PRINT0(_T("CreateThread(SENDER)"));
        hSenderThread = CreateThread(NULL, 0, &SENDER_ThreadProc, (LPVOID)hEventReady, 0, &idSenderThread);
        if (hSenderThread == NULL) {
            MAXEL = 0x0102; goto MAINWIN_WM_CREATE_EXCEPT;
        }
        DEBUG_PRINT0(_T("WaitForSignalObject(SENDER)"));
        ret = WaitForSingleObject(hEventReady, 2500);
        if (ret != 0) {
            MAXEL = 0x0103; goto MAINWIN_WM_CREATE_EXCEPT;
        }
        CloseHandle(hEventReady);
        hEventReady = NULL;

        DEBUG_PRINT0(_T("CreateThread(RECEIVER)"));
        hReceiverThread = CreateThread(NULL, 0, &RECEIVER_ThreadProc, (LPVOID)NULL, 0, &idReceiverThread);
        if (hReceiverThread == NULL) {
            MAXEL = 0x0104; goto MAINWIN_WM_CREATE_EXCEPT;
        }

        return 0;

    MAINWIN_WM_CREATE_EXCEPT:
        ret = GetLastError();

        if (hReceiverThread != NULL) {
            PostThreadMessage(idReceiverThread, WM_CLOSE, (WPARAM)0, (LPARAM)0);
            WaitForSingleObject(hReceiverThread, 2500);
            CloseHandle(hReceiverThread);
            hReceiverThread = NULL;
        }
        if (hSenderThread != NULL) {
            PostThreadMessage(idSenderThread, WM_CLOSE, (WPARAM)0, (LPARAM)0);
            WaitForSingleObject(hSenderThread, 2500);
            CloseHandle(hSenderThread);
            hSenderThread = NULL;
        }
        if (hEventReady != NULL) {
            CloseHandle(hEventReady);
        }

        showErrorDialog(0x01000000 | MAXEL, ret);
        PostQuitMessage(MAXEL);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/*
 * マウスイベントのフック
 */
LRESULT CALLBACK GLOBALHOOK_MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    static int emustate = ES_READY;            /* 実行状態 */
    static POINT pressed_start = {0, 0};       /* ホイールエミュレーション開始点 */
    static int emuwheel_vmoves_count = 0;      /* 縦 判定回数 */
    static int emuwheel_hmoves_count = 0;      /* 横 判定回数 */

    MSLLHOOKSTRUCT *pmh;
    int moves;
    BOOL found;
    INPUT *inputV, *inputH;
    EmuButton *eb;
    int btn;

    /* 無関係なものが飛び込んで来たら即座に明け渡す */
    if (nCode < 0 || nCode != HC_ACTION || wParam == WM_MOUSEWHEEL || wParam == WM_MOUSEHWHEEL) {
        goto ReturnNextHook;
    }

    pmh = (MSLLHOOKSTRUCT *)lParam;

    if (wParam == WM_MOUSEMOVE) {

        if (emustate == ES_READY) {
            goto ReturnNextHook;
        }

        if (emustate == ES_WHEELED) {
            DEBUG_PRINT0(_T("WHEEL ES_WHEELED"));
            emustate = ES_READY;
            goto ReturnNextHook;
        }

#ifdef DEBUG
        DEBUG_PRINT0(emustate == ES_PRESSED ? _T("WHEEL ES_PRESSED") : _T("WHEEL ES_WHEELING"));
#endif

        inputV = NULL;
        found = FALSE;
        moves = pmh->pt.y - pressed_start.y;
        DEBUG_PRINT1(_T("WHEEL"), DEBUG_INT, moves);
        if (moves < (emuwheel_vsens * -1)) {
            found = TRUE;
            emuwheel_vmoves_count++;
            DEBUG_PRINT1(_T("-"), DEBUG_INT, emuwheel_vmoves_count);
            if (emuwheel_vmoves_count > emuwheel_vsens_count) {
                inputV = &emuwheel_mi_up;
            }
        } else if (emuwheel_vsens < moves) {
            found = TRUE;
            emuwheel_vmoves_count--;
            DEBUG_PRINT1(_T("+"), DEBUG_INT, emuwheel_vmoves_count);
            if (emuwheel_vmoves_count < (emuwheel_vsens_count * -1)) {
                inputV = &emuwheel_mi_down;
            }
        }
        if (inputV != NULL) {
            emustate = ES_WHEELING;
            emuwheel_vmoves_count = 0;
            PostThreadMessage(idSenderThread, WM_EMUWHEEL, (WPARAM)0, (LPARAM)inputV);
        }

        if (emuwheel_horizontal_enabled) {
            inputH = NULL;
            moves = pmh->pt.x - pressed_start.x;
            DEBUG_PRINT1(_T("HWHEEL"), DEBUG_INT, moves);
            if (moves < (emuwheel_hsens * -1)) {
                found = TRUE;
                emuwheel_hmoves_count++;
                DEBUG_PRINT1(_T("-"), DEBUG_INT, emuwheel_hmoves_count);
                if (emuwheel_hmoves_count > emuwheel_hsens_count) {
                    inputH = &emuwheel_mi_left;
                }
            } else if (emuwheel_hsens < moves) {
                found = TRUE;
                emuwheel_hmoves_count--;
                DEBUG_PRINT1(_T("+"), DEBUG_INT, emuwheel_hmoves_count);
                if (emuwheel_hmoves_count < (emuwheel_hsens_count * -1)) {
                    inputH = &emuwheel_mi_right;
                }
            }
            if (inputH != NULL) {
                emustate = ES_WHEELING;
                emuwheel_hmoves_count = 0;
                PostThreadMessage(idSenderThread, WM_EMUWHEEL, (WPARAM)0, (LPARAM)inputH);
            }
        }

        /* カーソル位置を固定 */
        if (found) {
            SetCursorPos(pressed_start.x, pressed_start.y);
        }

        return 1;
    }

    if (wParam == emuwheel_button_wm_down) { /* ホイール開始 */
        if ( (wParam == (UINT_PTR)WM_XBUTTONDOWN)
             && ((DWORD)GET_XBUTTON_WPARAM(pmh->mouseData) != emuwheel_button_wm_x) ) {
            ;;

        } else if (emustate == ES_READY) {
            DEBUG_PRINT0(_T("BTN DOWN ES_READY"));
            /* 位置決め */
            pressed_start.x = pmh->pt.x;
            pressed_start.y = pmh->pt.y;

            emuwheel_mi_up.mi.dx = pressed_start.x;
            emuwheel_mi_up.mi.dy = pressed_start.y;
            emuwheel_mi_down.mi.dx = pressed_start.x;
            emuwheel_mi_down.mi.dy = pressed_start.y;

            if (emuwheel_horizontal_enabled) {
                emuwheel_mi_left.mi.dx = pressed_start.x;
                emuwheel_mi_left.mi.dy = pressed_start.y;
                emuwheel_mi_right.mi.dx = pressed_start.x;
                emuwheel_mi_right.mi.dy = pressed_start.y;
            }

            if (emuwheel_click_enabled) {
                emuwheel_button_mi_down.mi.dx = pressed_start.x;
                emuwheel_button_mi_down.mi.dy = pressed_start.y;
                emuwheel_button_mi_up.mi.dx = pressed_start.x;
                emuwheel_button_mi_up.mi.dy = pressed_start.y;
            }

            emuwheel_vmoves_count = 0;
            emuwheel_hmoves_count = 0;

            emustate = ES_PRESSED;
            return 1;

        } else if (emustate == ES_WHEELED) {
            DEBUG_PRINT0(_T("BTN DOWN ES_WHEELED"));

        } else {
            /* DOWNが連続する異常値は無視 */
            DEBUG_PRINT0(_T("BTN DOWN ES_*"));
            return 1;
        }

    } else if (wParam == emuwheel_button_wm_up) { /* ホイール終了 */
        if ( (wParam == (UINT_PTR)WM_XBUTTONUP)
             && ((DWORD)GET_XBUTTON_WPARAM(pmh->mouseData) != emuwheel_button_wm_x) ) {
            ;;

        } else if (emustate == ES_READY) {
            /* UPから始まってしまった場合は無視 */
            DEBUG_PRINT0(_T("BTN UP ES_READY"));
            if (emuwheel_click_enabled == 0) {
                return 1;
            }

        } else if (emustate == ES_PRESSED) {
            if (emuwheel_click_enabled) {
                /* ホイールせずにUpしたらClick */
                DEBUG_PRINT0(_T("BTN UP ES_PRESSED CLICK"));
                emustate = ES_WHEELED;
                PostThreadMessage(idSenderThread, WM_EMUCLICK, (WPARAM)0, (LPARAM)0);
            } else {
                DEBUG_PRINT0(_T("BTN UP ES_PRESSED -"));
                emustate = ES_READY;
            }
            return 1;

        } else if (emustate == ES_WHEELING) {
            DEBUG_PRINT0(_T("BTN UP ES_WHEEING"));
            emustate = ES_READY;
            return 1;

        } else if (emustate == ES_WHEELED) {
            DEBUG_PRINT0(_T("BTN UP ES_WHEELED"));
            emustate = ES_READY;

        } else {
            return 1;
        }

    }

    /* 同時押し処理 */
    if (combo_button1_wm_down != 0) {

        /* 押されたボタンを検出 */
        btn = 0;
        if (wParam == combo_button1_wm_down) {
            if (wParam == combo_button2_wm_down) { /* #1==#2==XBUTTON */
                if ((DWORD)GET_XBUTTON_WPARAM(pmh->mouseData) == combo_button1_mi_down.mi.mouseData) {
                    btn = 0x01;
                } else if ((DWORD)GET_XBUTTON_WPARAM(pmh->mouseData) == combo_button2_mi_down.mi.mouseData) {
                    btn = 0x02;
                }
            } else if (wParam == (UINT_PTR)WM_XBUTTONDOWN && (DWORD)GET_XBUTTON_WPARAM(pmh->mouseData) != combo_button1_mi_down.mi.mouseData) {
                ;;
            } else {
                btn = 0x01;
            }

        } else if (wParam == combo_button2_wm_down) {
            if (wParam == (UINT_PTR)WM_XBUTTONDOWN && (DWORD)GET_XBUTTON_WPARAM(pmh->mouseData) != combo_button2_mi_down.mi.mouseData) {
                ;;
            } else {
                btn = 0x02;
            }

        } else if (wParam == combo_button1_wm_up) {
            if (wParam == combo_button2_wm_up) { /* #1==#2==XBUTTON */
                if ((DWORD)GET_XBUTTON_WPARAM(pmh->mouseData) == combo_button1_mi_up.mi.mouseData) {
                    btn = 0x11;
                } else if ((DWORD)GET_XBUTTON_WPARAM(pmh->mouseData) == combo_button2_mi_up.mi.mouseData) {
                    btn = 0x12;
                }
            } else if (wParam == (UINT_PTR)WM_XBUTTONUP && (DWORD)GET_XBUTTON_WPARAM(pmh->mouseData) != combo_button2_mi_up.mi.mouseData) {
                ;;
            } else {
                btn = 0x11;
            }

        } else if (wParam == combo_button2_wm_up) {
            if (wParam == (UINT_PTR)WM_XBUTTONUP && (DWORD)GET_XBUTTON_WPARAM(pmh->mouseData) != combo_button2_mi_up.mi.mouseData) {
                ;;
            } else {
                btn = 0x12;
            }
        }

        /* ボタン毎の処理 */
        if (btn == 0) {
            ;;

        } else if (btn == 0x01) {
            if (combo_button1_down) {
                combo_button1_down = FALSE;
            } else {
                DEBUG_PRINT1(_T("COMBO DOWN #1"), DEBUG_HEX, btnState);
                btnState = btnState | 0x01;
                if (btnState == 0x03) {
                    pressed_combo_buttons = 1;
                    combo_mi_down.mi.dx = pmh->pt.x;
                    combo_mi_down.mi.dy = pmh->pt.y;
                    DEBUG_PRINT0(_T("+"));
                    PostThreadMessage(idSenderThread, WM_COMBODOWN, (WPARAM)0, (LPARAM)0);
                } else {
                    DEBUG_PRINT0(_T("-"));
                    combo_button1_mi_down.mi.dx = pmh->pt.x;
                    combo_button1_mi_down.mi.dy = pmh->pt.y;
                    ResetEvent(hEventComboButton1Up);
                    PostThreadMessage(idSenderThread, WM_COMBOBTN1DOWN, (WPARAM)0, (LPARAM)0);
                }
                return 1;
            }

        } else if (btn == 0x02) {
            if (combo_button2_down) {
                combo_button2_down = FALSE;
            } else {
                DEBUG_PRINT1(_T("COMBO DOWN #2"), DEBUG_HEX, btnState);
                btnState = btnState | 0x02;
                if (btnState == 0x03) {
                    pressed_combo_buttons = 1;
                    combo_mi_down.mi.dx = pmh->pt.x;
                    combo_mi_down.mi.dy = pmh->pt.y;
                    DEBUG_PRINT0(_T("+"));
                    PostThreadMessage(idSenderThread, WM_COMBODOWN, (WPARAM)0, (LPARAM)0);
                } else {
                    DEBUG_PRINT0(_T("-"));
                    combo_button2_mi_down.mi.dx = pmh->pt.x;
                    combo_button2_mi_down.mi.dy = pmh->pt.y;
                    ResetEvent(hEventComboButton2Up);
                    PostThreadMessage(idSenderThread, WM_COMBOBTN2DOWN, (WPARAM)0, (LPARAM)0);
                }
                return 1;
            }

        } else if (btn == 0x11) {
            if (combo_button1_up) {
                combo_button1_up = FALSE;
            } else {
                DEBUG_PRINT1(_T("COMBO UP #1"), DEBUG_HEX, btnState);
                btnState = btnState & 0x02;
                if (pressed_combo_buttons) {
                    if ((btnState & 0x02) != 0) {
                        combo_mi_up.mi.dx = pmh->pt.x;
                        combo_mi_up.mi.dy = pmh->pt.y;
                        DEBUG_PRINT0(_T("+"));
                        PostThreadMessage(idSenderThread, WM_COMBOUP, (WPARAM)0, (LPARAM)0);
                    } else {
                        DEBUG_PRINT0(_T("-"));
                        pressed_combo_buttons = 0;
                    }
                } else {
                    combo_button1_mi_up.mi.dx = pmh->pt.x;
                    combo_button1_mi_up.mi.dy = pmh->pt.y;
                    SetEvent(hEventComboButton1Up);
                    PostThreadMessage(idSenderThread, WM_COMBOBTN1UP, (WPARAM)0, (LPARAM)0);
                }
                return 1;
            }

        } else if (btn == 0x12) {
            if (combo_button2_up) {
                combo_button2_up = FALSE;
            } else {
                DEBUG_PRINT1(_T("COMBO UP #2"), DEBUG_HEX, btnState);
                btnState = btnState & 0x01;
                if (pressed_combo_buttons) {
                    if ((btnState & 0x01) != 0) {
                        combo_mi_up.mi.dx = pmh->pt.x;
                        combo_mi_up.mi.dy = pmh->pt.y;
                        DEBUG_PRINT0(_T("+"));
                        PostThreadMessage(idSenderThread, WM_COMBOUP, (WPARAM)0, (LPARAM)0);
                    } else {
                        DEBUG_PRINT0(_T("-"));
                        pressed_combo_buttons = 0;
                    }
                } else {
                    combo_button2_mi_up.mi.dx = pmh->pt.x;
                    combo_button2_mi_up.mi.dy = pmh->pt.y;
                    SetEvent(hEventComboButton2Up);
                    PostThreadMessage(idSenderThread, WM_COMBOBTN2UP, (WPARAM)0, (LPARAM)0);
                }
                return 1;
            }

        }
    }

    /* 読み替え中のボタンは読み替えない */
    if ((pmh->flags & (LLMHF_INJECTED | LLMHF_LOWER_IL_INJECTED)) != 0) {
        DEBUG_PRINT1(_T("."), DEBUG_HEX, wParam);
        goto ReturnNextHook;
    }
    /* ボタン読み替え */
    for (eb = emubuttons; eb != NULL; eb = eb->next) {
        if (wParam == eb->src) {
            if ( (wParam == (UINT_PTR)WM_XBUTTONDOWN || wParam == (UINT_PTR)WM_XBUTTONUP)
                 && ((DWORD)GET_XBUTTON_WPARAM(pmh->mouseData) != eb->src_x) ) {
                continue;
            }
            DEBUG_PRINT0(_T("ALT"));
            eb->i.mi.dx = pmh->pt.x;
            eb->i.mi.dy = pmh->pt.y;
            PostThreadMessage(idSenderThread, WM_ALTBUTTON, wParam, (LPARAM)eb);
            return 1;
        }
    }

ReturnNextHook:
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

/*
 * マウスメッセージ受信スレッド
 */
DWORD WINAPI RECEIVER_ThreadProc(LPVOID lpParam)
{
    int MAXEL = 0;
    HHOOK hMouseHook;
    MSG msg;
    int ret;

    /* フックのためにメッセージキューが必要 */
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

    /* フックを仕掛ける */
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, GLOBALHOOK_MouseProc, GetModuleHandle(NULL), 0);
    if (hMouseHook == NULL) {
        showErrorDialog(0x02000201, GetLastError());
        MAXEL = 0x0201; goto FINALLY;
    }
    DEBUG_PRINT1(_T("MOUSE HOOKED"), DEBUG_HEX, hMouseHook);

    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (ret == -1) {
            MAXEL = 0x0251;
            break;
        }
        switch (msg.message) {
        case WM_CLOSE:
        case WM_DESTROY:
            DEBUG_PRINT0(_T("CLOSE RECEIVER"));
            PostQuitMessage(0);
            break;
        }
        DispatchMessage(&msg);
    }
    if (ret == 0) {
        MAXEL = msg.wParam;
    }

FINALLY:
    DEBUG_PRINT0(_T("CLOSING RECEIVER THREAD"));

    if (hMouseHook != NULL) {
        UnhookWindowsHookEx(hMouseHook);
    }

    ExitThread(MAXEL);
    return 0;
}

/*
 * マウスメッセージ送信スレッド
 */
DWORD WINAPI SENDER_ThreadProc(LPVOID lpParam)
{
    int MAXEL = 0;
    MSG msg;
    int ret;

    /* 同時押し処理のために使うフラグ準備 */
    if (combo_button1_wm_down != 0) {
        DEBUG_PRINT0(_T("SENDER CreateEvent(#1)"));
        hEventComboButton1Up = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hEventComboButton1Up == NULL) {
            MAXEL = 0x0301; goto FINALLY;
        }
        DEBUG_PRINT0(_T("SENDER CreateEvent(#2)"));
        hEventComboButton2Up = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hEventComboButton2Up == NULL) {
            MAXEL = 0x0302; goto FINALLY;
        }
    } else {
        hEventComboButton1Up = NULL;
        hEventComboButton2Up = NULL;
    }

    /* メッセージキュー */
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

    /* 呼び出し元へ準備完了を伝達 */
    if (lpParam != NULL) {
        SetEvent((HANDLE)lpParam);
    }

    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (ret == -1) {
            MAXEL = 0x351;
            break;
        }
        switch (msg.message) {
        case WM_EMUWHEEL:
            DEBUG_PRINT0(_T("SEND WHEEL"));
            SendInput(1, (INPUT *)msg.lParam, sizeof(INPUT));
            break;

        case WM_ALTBUTTON:
#ifdef DEBUG
            switch (msg.wParam){
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_XBUTTONDOWN:
                DEBUG_PRINT0(_T("SEND ALT DOWN"));
                break;
            default:
                DEBUG_PRINT0(_T("SEND ALT UP"));
            }
#endif
            SendInput(1, &((EmuButton *)msg.lParam)->i, sizeof(INPUT));
            break;

        case WM_EMUCLICK:
            DEBUG_PRINT0(_T("SEND EMUCLICK"));
            SendInput(1, &emuwheel_button_mi_down, sizeof(INPUT));
            SendInput(1, &emuwheel_button_mi_up, sizeof(INPUT));
            break;

        case WM_COMBOBTN1DOWN:
            if (hEventComboButton1Up == NULL)
                break;
            WaitForSingleObject(hEventComboButton1Up, combo_button_time);
            if (! pressed_combo_buttons) {
                DEBUG_PRINT0(_T("SEND COMBO-BUTTON-1 DOWN"));
                btnState = 0;
                combo_button1_down = TRUE;
                SendInput(1, &combo_button1_mi_down, sizeof(INPUT));
            }
            break;

        case WM_COMBOBTN2DOWN:
            if (hEventComboButton2Up == NULL)
                break;
            WaitForSingleObject(hEventComboButton2Up, combo_button_time);
            if (! pressed_combo_buttons) {
                DEBUG_PRINT0(_T("SEND COMBO-BUTTON-2 DOWN"));
                btnState = 0;
                combo_button2_down = TRUE;
                SendInput(1, &combo_button2_mi_down, sizeof(INPUT));
            }
            break;

        case WM_COMBOBTN1UP:
            DEBUG_PRINT0(_T("SEND COMBO-BUTTON-1 UP"));
            combo_button1_up = TRUE;
            SendInput(1, &combo_button1_mi_up, sizeof(INPUT));
            break;

        case WM_COMBOBTN2UP:
            DEBUG_PRINT0(_T("SEND COMBO-BUTTON-2 UP"));
            combo_button2_up = TRUE;
            SendInput(1, &combo_button2_mi_up, sizeof(INPUT));
            break;

        case WM_COMBODOWN:
            DEBUG_PRINT0(_T("SEND COMBO DOWN"));
            SendInput(1, &combo_mi_down, sizeof(INPUT));
            break;

        case WM_COMBOUP:
            DEBUG_PRINT0(_T("SEND COMBO UP"));
            SendInput(1, &combo_mi_up, sizeof(INPUT));
            break;

        case WM_CLOSE:
        case WM_DESTROY:
            DEBUG_PRINT0(_T("CLOSE SENDER"));
            PostQuitMessage(0);
            break;
        }
    }
    if (ret == 0) {
        MAXEL = msg.wParam;
    }

FINALLY:
    DEBUG_PRINT0(_T("CLOSING SENDER THREAD"));

    if (hEventComboButton1Up != NULL) {
        CloseHandle(hEventComboButton1Up);
    }
    if (hEventComboButton2Up != NULL) {
        CloseHandle(hEventComboButton2Up);
    }

    ExitThread(MAXEL);
    return 0;
}

#ifdef __TINYC__ && _WIN64
/*
 * Startup routine for Tiny C
 */
void __stdcall _wwinstart();
void __stdcall _wwinstart()
{
    register TCHAR *cl = GetCommandLine();
    register BOOL dq = FALSE;

    for (; *cl != _T('\0'); cl++) {
        if (*cl == _T('"')) {
            dq = !dq;
        } else if (*cl == _T(' ')) {
            if (!dq) {
                cl++;
                break;
            }
        }
    }

    ExitProcess( _tWinMain(GetModuleHandle(NULL), NULL, cl, 0) );
}
#endif

/*
 * WinMain
 */
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    int MAXEL = 0;

    WNDCLASS wc;
    ATOM aMainWin = 0;
    HWND hMainWin = NULL;

    MSG msg;
    int ret;
    HWND hret;

    NOTIFYICONDATA nid;
    HICON hMainTrayIcon = NULL;
    BOOL trayIconEnabled = FALSE;

#ifdef DEBUG
    AllocConsole();
    DEBUG_PRINT0(_T("START"));
    DEBUG_PRINT1(_T("hInstance"), DEBUG_HEX, hInstance);
    DEBUG_PRINT1(_T("lpCmdLine"), DEBUG_STR, lpCmdLine);
#endif

    /* 設定 */
    if ((ret = parseCommandLine(lpCmdLine)) != 0) {
        if (ret == CMDRET_QUIT) {
            hret = FindWindow(_T(MAINWIN_CLASSNAME), NULL);
            if (hret != NULL) {
                PostMessage(hret, WM_CLOSE, (WPARAM)0, (LPARAM)0);
            }
            goto FINALLY;
        }
        showErrorDialog(0x00010000 | ret, GetLastError());
        MAXEL = 1; goto FINALLY;
    }

    /* 既に実行中であれば終了 */
    if (FindWindow(_T(MAINWIN_CLASSNAME), NULL) != NULL) {
        goto FINALLY;
    }

    /* 便宜上の不可視メインウィンドウを作成 */
    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.lpfnWndProc     = &MAINWIN_WndProc;
    wc.hInstance       = hInstance;
    wc.hIcon           = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground   = (HBRUSH)COLOR_APPWORKSPACE;
    wc.lpszClassName   = _T(MAINWIN_CLASSNAME);
    aMainWin = RegisterClass(&wc);
    if (aMainWin == 0) {
        showErrorDialog(0x00020001, GetLastError());
        MAXEL = 21; goto FINALLY;
    }
    hMainWin = CreateWindow(_T(MAINWIN_CLASSNAME)
                            , strAppTitle
                            , 0
                            , 0, 0, 1, 1
                            , NULL
                            , NULL
                            , hInstance
                            , NULL);
    if (hMainWin == NULL) {
        showErrorDialog(0x00020002, GetLastError());
        MAXEL = 22; goto FINALLY;
    }

    /* トレイ用アイコン画像を読み込む */
    hMainTrayIcon = ExtractIcon(hInstance, _T("main.cpl"), 0);
    /*
    if (hMainTrayIcon == NULL) {
        hMainTrayIcon = ExtractIcon(hInstance, _T("setupapi.dll"), 1);
        if (hMainTrayIcon == NULL) {
            hMainTrayIcon = ExtractIcon(hInstance, _T("mmcndmgr.dll"), 50);
        }
    } */

    /* トレイアイコンを作成 */
    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
    nid.cbSize           = sizeof(NOTIFYICONDATA);
    nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.hWnd             = hMainWin;
    nid.hIcon            = hMainTrayIcon;
    nid.uID              = MAINICON_ID;
    nid.uCallbackMessage = WM_MAINICON;
    lstrcpy(nid.szTip, strAppTitle);
    trayIconEnabled = Shell_NotifyIcon(NIM_ADD, &nid);
    if (! trayIconEnabled) {
        showErrorDialog(0x00020004, GetLastError());
        MAXEL = 24; goto FINALLY;
    }

    /* メインスレッドのメッセージループ */
    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (ret == -1) {
            showErrorDialog(0x000500001, GetLastError());
            MAXEL = 51;
            break;
        }
        DispatchMessage(&msg);
    }
    if (ret == 0) {
        MAXEL = msg.wParam;
    }

FINALLY:

    if (trayIconEnabled) {
        ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd   = hMainWin;
        nid.uID    = MAINICON_ID;
        Shell_NotifyIcon(NIM_DELETE, &nid);
    }

    if (hMainTrayIcon != NULL) {
        DestroyIcon(hMainTrayIcon);
    }

    if (aMainWin != 0) {
        UnregisterClass(_T(MAINWIN_CLASSNAME), hInstance);
    }

    if (emubuttons) {
        freeEmuButtons();
    }

#ifdef DEBUG
    FreeConsole();
#endif

    return MAXEL;
}
