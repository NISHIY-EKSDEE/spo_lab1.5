#include <netinet/in.h>
#include <arpa/inet.h>
#include "client.h"
#include "../utils/my_alloc.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/keysymdef.h>

#define X 0
#define Y 0
#define WIDTH 1280
#define HEIGHT 720
#define WIDTH_MIN 1280
#define HEIGHT_MIN 720
#define BORDER_WIDTH 5
#define TITLE "Neo4J"
#define ICON_TITLE "Neo4J"
#define PRG_CLASS "Neo4J"

static void SetWindowManagerHints(
    Display *display, /*Указатель на структуру Display */
    char *PClass,     /*Класс программы */
    char *argv[],     /*Аргументы программы */
    int argc,         /*Число аргументов */
    Window window,    /*Идентификатор окна */
    int x,            /*Координаты левого верхнего */
    int y,            /*угла окна */
    int win_wdt,      /*Ширина  окна */
    int win_hgt,      /*Высота окна */
    int win_wdt_min,  /*Минимальная ширина окна */
    int win_hgt_min,  /*Минимальная высота окна */
    char *ptrTitle,   /*Заголовок окна */
    char *ptrITitle,  /*Заголовок пиктограммы окна */
    Pixmap pixmap     /*Рисунок пиктограммы */
)
{
    XSizeHints size_hints; /*Рекомендации о размерах окна*/

    XWMHints wm_hints;
    XClassHint class_hint;
    XTextProperty windowname, iconname;

    if (!XStringListToTextProperty(&ptrTitle, 1, &windowname) ||
        !XStringListToTextProperty(&ptrITitle, 1, &iconname))
    {
        puts("No memory!\n");
        exit(1);
    }

    size_hints.flags = PPosition | PSize | PMinSize;
    size_hints.min_width = win_wdt_min;
    size_hints.min_height = win_hgt_min;
    wm_hints.flags = StateHint | IconPixmapHint | InputHint;
    wm_hints.initial_state = NormalState;
    wm_hints.input = True;
    wm_hints.icon_pixmap = pixmap;
    class_hint.res_name = argv[0];
    class_hint.res_class = PClass;

    XSetWMProperties(display, window, &windowname,
                     &iconname, argv, argc, &size_hints, &wm_hints,
                     &class_hint);
}

void redraw_window(GC gc, Display *display, Window window, char* input, char* output) {
    XClearWindow(display, window);
    XDrawLine(display, window, gc, 0, 450, 1280, 450);
    XDrawString ( display, window, gc, 20,500,
        input, strlen (input));

    char* message = strdup(output);
    char* pch = NULL;

    pch = strtok(message, "\n");
    int i = 0;
    while (pch != NULL)
    {
        XDrawString ( display, window, gc, 20,50 + i*15, pch, strlen(pch));
        pch = strtok(NULL, "\n");
        i++;
    }
    free(message);
    free(pch);
}

int main(int argc, char **argv)
{
    int width = WIDTH, height = HEIGHT;
    Display *display; /* Указатель на структуру Display */
    int ScreenNumber; /* Номер экрана */
    GC gc;            /* Графический контекст */
    XEvent event;
    Window window;

    if (argc < 2)
    {
        puts("No required arguments provided: <server_port>");
        return -1;
    }
    uint16_t port = strtoul(argv[1], NULL, BASE_10);
    int32_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(HOSTNAME);

    int connection = connect(server_fd, (const struct sockaddr *)&server_address, sizeof(server_address));
    if (connection == -1)
    {
        puts("There was an error making a connection to the remote socket");
        return -1;
    }

    /* Устанавливаем связь с сервером */
    if ((display = XOpenDisplay(NULL)) == NULL)
    {
        puts("Can not connect to the X server!\n");
        exit(1);
    }

    /* Получаем номер основного экрана */
    ScreenNumber = DefaultScreen(display);

    /* Создаем окно */
    window = XCreateSimpleWindow(display,
                                 RootWindow(display, ScreenNumber),
                                 X, Y, WIDTH, HEIGHT, BORDER_WIDTH,
                                 BlackPixel(display, ScreenNumber),
                                 WhitePixel(display, ScreenNumber));

    /* Задаем рекомендации для менеджера окон */
    SetWindowManagerHints(display, PRG_CLASS, argv, argc,
                          window, X, Y, WIDTH, HEIGHT, WIDTH_MIN,
                          HEIGHT_MIN, TITLE, ICON_TITLE, 0);

    /* Выбираем события,  которые будет обрабатывать программа */
    XSelectInput(display, window, ExposureMask|StructureNotifyMask|KeyPressMask|KeymapStateMask);

    /* Покажем окно */
    XMapWindow(display, window);

    gc = XCreateGC ( display, window, 0 , NULL );
    XSetForeground ( display, gc, BlackPixel ( display, 0) );

    char *input = NULL;
    size_t length;
    uint8_t errors;
    puts("Enter CYPHER query or type \"exit\" to leave");
    long count = server_fd;
    char *response = my_alloc(BUFSIZ);
    char response_string[RESPONSE_BUFFER_SIZE] = {0};
    char *cmd = malloc(1024);
    bzero(cmd, 1024);

    while (count > 0)
    {
        // getline(&input, &length, stdin);
        int flag = 1;
        while(flag) {
            XNextEvent(display, &event);

            switch(event.type){
            case Expose:
                /* Запрос на перерисовку */
                if (event.xexpose.count != 0)
                    break;
                
                redraw_window(gc, display, window, cmd, &response_string);
                break;
            case KeymapNotify:
                XRefreshKeyboardMapping(&event.xmapping);
                break;
            case KeyPress: 
                {
                    char string[1];
                    int len;
                    KeySym keysym;
                    len = XLookupString(&event.xkey, string, 1, &keysym, NULL);

                    switch(keysym){
                        case XK_BackSpace:
                        {
                            int pos = strlen(cmd) > 0 ? strlen(cmd) - 1 : 0;
                            cmd[pos] = '\0';
                        }
                            break;
                        case XK_Return:
                        {
                            if(strlen(cmd) > 0) {
                                input = strdup(cmd);
                                bzero(cmd, 1024);
                                flag = 0;
                            }
                        }
                            break;
                        case XK_Shift_L:
                            break;
                        default:
                        {
                            strncat(cmd, &(string[0]), 1);
                        }

                    }

                    redraw_window(gc, display, window, cmd, &response_string);
                }
                break;
            case ConfigureNotify:
                if (width != event.xconfigure.width
                        || height != event.xconfigure.height) {
                    width = event.xconfigure.width;
                    height = event.xconfigure.height;
                    printf("Size changed to: %d by %d\n", width, height);
                }
                break;
            }
        }
        puts(input);
        if (strcmp(input, "exit") == 0)
            break;
        cypher_parse_result_t *result = cypher_parse(
            input, NULL, NULL, CYPHER_PARSE_ONLY_STATEMENTS);
        if (result == NULL)
        {
            perror("cypher_parse");
            return EXIT_FAILURE;
        }
        puts("");
        cypher_parse_result_fprint_ast(result, stdout, 100, NULL, 0);
        puts("");
        errors = cypher_parse_result_nerrors(result);
        if (errors > 0)
        {
            puts("Unknown command");
            cypher_parse_result_free(result);
            continue;
        }
        query_info *info = get_query_info(result);
        if (info == NULL)
            continue;
        char *request = build_client_json_request(info);
        puts("Request:");
        puts(request);
        puts("");
        send_message(server_fd, request);
        bzero(response, BUFSIZ);
        count = recv(server_fd, response, BUFSIZ, 0);
        long content_length = strtol(response, NULL, 10);
        char *response_json = receive_message(server_fd, content_length);

        puts("Response:");
        puts(response_json);
        puts("");
        bzero(response_string, RESPONSE_BUFFER_SIZE);
        parse_json_response(response_json, response_string);

        puts(response_string);
        redraw_window(gc, display, window, cmd, &response_string);
        puts("");
        free_query_info(info);
        cypher_parse_result_free(result);
    }
}