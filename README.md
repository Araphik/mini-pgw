# Описание

## 1. Клиент
```plaintext
Клиент представляет собой клиентское приложение для отправки данных IMSI (международного идентификатора мобильного абонента) на сервер по протоколу UDP с поддержкой загрузки конфигурации и ведения логов.
```

---

### 1.1. bcd_encoder.cpp
```plaintext
Определяет класс BcdEncoder, который предоставляет метод для кодирования строки IMSI в формат Binary-Coded Decimal (BCD). Функция encode принимает строку IMSI и обрабатывает её по две цифры за раз, преобразуя каждую пару в один байт. Первая цифра занимает младшие 4 бита, а вторая цифра (если она есть) — старшие 4 бита. Если длина IMSI нечётная, последний байт дополняется значением 0xF в старших 4 битах. Результат кодирования возвращается в виде вектора байтов.
```
### 1.2. pgw_client.cpp
```plaintext
Реализует класс ImsiClient, который отвечает за отправку данных IMSI на сервер по протоколу UDP. Конструктор принимает объект ClientConfig и опциональные флаги для имитации сбоев при создании сокета, отправке, выборе или получении данных для целей тестирования. Метод send_imsi_request кодирует IMSI с помощью класса BcdEncoder, создаёт неблокирующий UDP-сокет и настраивает адрес сервера, используя IP и порт из конфигурации. Он пытается отправить закодированный IMSI на сервер до максимального количества попыток (MAX_RETRIES). На каждой попытке данные отправляются, ожидается ответ с использованием select с таймаутом в 5 секунд, и результат логируется. Если ответ получен, он записывается в лог, и попытка считается успешной. Если все попытки исчерпаны или произошла ошибка, выводится сообщение об ошибке, и сокет закрывается.
```
### 1.3. logger_initializer.cpp 
```plaintext
Определяет класс LoggerInitializer, который настраивает систему постоянного логирования с использованием spdlog. Метод initialize принимает объект ClientConfig и создаёт два приёмника логов: один для записи в файл, указанный в конфигурации, и другой для цветного вывода в консоль. Эти приёмники объединяются в единый логгер с именем client_logger, который устанавливается как логгер по умолчанию. Уровень логирования задаётся на основе поля log_level конфигурации. Если уровень логирования недопустим, выбрасывается исключение. Логгер настроен на сброс сообщений на уровне info и использует формат логов, включающий временные метки и цветные уровни логов.
```
### 1.4. main.cpp 
```plaintext
Файл main.cpp содержит точку входа приложения. Он инициализирует временный логгер с использованием библиотеки spdlog для вывода логов в консоль с заданным форматом, включающим временные метки и уровни логов. Программа ожидает один аргумент командной строки — строку IMSI. Она загружает конфигурацию клиента из JSON-файла (client_config.json) с помощью функции load_client_config. Если загрузка конфигурации не удалась, выводится критическая ошибка, и программа завершается. После успешной загрузки конфигурации инициализируется постоянный логгер с помощью класса LoggerInitializer, создаётся экземпляр ImsiClient для отправки IMSI на сервер. Программа логирует прогресс и обрабатывает ошибки.
```
## 2. Сервер
```plaintext
Сервер представляет собой приложение, которое принимает данные IMSI (международного идентификатора мобильного абонента) по протоколу UDP, обрабатывает их, управляет сессиями, поддерживает HTTP API для проверки статуса абонентов и завершения работы, а также записывает события в CDR-файл. Приложение использует многопоточность, логирование и обработку сигналов для надёжной работы.
```
### 2.1. cdr_writer.cpp 
```plaintext
Определяет класс CdrWriter, который отвечает за запись событий в файл CDR (Call Detail Record). Конструктор принимает путь к файлу CDR. Метод write записывает в файл строку, содержащую временную метку, IMSI и действие (например, "create" или "timeout"). Временная метка формируется в формате "ГГГГ-ММ-ДД ЧЧ:ММ:СС" с использованием текущего времени. Если файл не удаётся открыть или записать, логируется ошибка с помощью spdlog. При успешной записи событие логируется на уровне debug.
```
### 2.2. logger_initializer.cpp 
```plaintext
Определяет класс LoggerInitializer для настройки системы логирования с использованием spdlog. Конструктор принимает объект ServerConfig и создаёт директорию для логов (../logs), если она отсутствует. Затем создаётся файловый приёмник логов на основе пути, указанного в конфигурации, и добавляется к существующему логгеру. Уровень логирования устанавливается согласно log_level из конфигурации. Если указан некорректный уровень, он сбрасывается на info, и логируется предупреждение. При ошибке инициализации выбрасывается исключение.
```
### 2.3. signal_handler.cpp
```plaintext
Реализует класс SignalHandler для обработки сигналов, таких как SIGINT (Ctrl+C). Конструктор регистрирует статический метод handle для обработки сигналов. При получении SIGINT программа логирует сообщение о завершении, вызывает spdlog::shutdown и завершает выполнение. Для неподдерживаемых сигналов логируется предупреждение.
```
### 2.4. decode_utils.cpp 
```plaintext
Содержит класс BcdDecoder, который декодирует данные из формата Binary-Coded Decimal (BCD) в строку IMSI. Метод decode принимает вектор байтов, извлекает младшие и старшие 4 бита каждого байта, преобразует их в цифры (если значение меньше 10) и формирует строку IMSI.
```
### 2.5. http_api.cpp
```plaintext
Реализует класс HttpServer, который предоставляет HTTP API для проверки статуса абонентов и остановки сервера. Конструктор принимает конфигурацию, флаг выполнения, таблицу сессий, мьютекс и дескриптор события. Метод run запускает HTTP-сервер в отдельном потоке с использованием библиотеки httplib. Сервер обрабатывает GET-запросы к /check_subscriber для проверки статуса IMSI (возвращает "active" или "not active") и POST-запросы к /stop для остановки сервера. При вызове /stop сервер записывает сигнал в event_fd и останавливается. Метод stop корректно завершает работу HTTP-сервера.
```
### 2.6. pgw_server.cpp
```plaintext
Реализует класс PgwServer, который управляет основным функционалом сервера. Конструктор инициализирует конфигурацию, объект CdrWriter, флаг выполнения, пул потоков и чёрный список IMSI. Метод setup_socket создаёт неблокирующий UDP-сокет и привязывает его к IP и порту из конфигурации. Метод setup_epoll настраивает epoll для обработки входящих данных. Метод start_session_cleaner запускает поток для удаления истёкших сессий на основе таймаута из конфигурации. Метод start_thread_pool создаёт пул потоков для обработки клиентских запросов. Метод worker_thread обрабатывает задачи из очереди, вызывая handle_client для декодирования IMSI, проверки чёрного списка и управления сессиями. Метод run запускает сервер, используя epoll для обработки входящих UDP-пакетов и событий от HTTP API, и корректно завершает работу при получении сигнала остановки.
```
### 2.7. main.cpp 
```plaintext
Содержит точку входа серверного приложения. Он инициализирует обработчик сигналов для перехвата SIGINT (Ctrl+C), загружает конфигурацию сервера из файла server_config.json с помощью функции load_config_server. Если загрузка конфигурации не удалась, программа логирует критическую ошибку и завершается. Затем инициализируется логгер с помощью класса LoggerInitializer. После этого создаётся экземпляр класса PgwServer, который запускает UDP-сервер. Программа логирует запуск и завершение сервера, а также обрабатывает исключения, возникающие во время работы.
```
# Тесты 

## *Как запустить?*
```bash
./build/unit_test
```
## *Клиент*

### 1. test_pgw_client.cpp

Cодержит юнит-тесты для класса PgwClient, который отвечает за отправку IMSI-запросов и получение ответов по протоколу UDP.

#### 1.1. *SendsAndReceivesSuccessfully*

Тест проверяет, что клиент ImsiClient успешно отправляет IMSI-запрос на тестовый UDP-сервер, запущенный на порту 9999, и получает ответ. В логах проверяется наличие строки "Received response", что подтверждает успешное взаимодействие.

#### 1.2. *RetriesOnTimeoutAndFails*

Тест проверяет поведение клиента при отправке IMSI на несуществующий сервер на порту 9998. Клиент должен выполнять повторные попытки отправки и завершать их с ошибкой после исчерпания попыток, а в логах должна появиться строка "Max retries reached, giving up".

#### 1.3. *InvalidIpFailsGracefully*

Тест проверяет, что клиент корректно обрабатывает невалидный IP-адрес (256.256.256.256), завершая работу с ошибкой. В логах ожидается строка "Invalid server IP address".

#### 1.4. *LogsImsiBeingSent*

Тест подтверждает, что клиент записывает в лог отправляемый IMSI (111111111111111) при взаимодействии с сервером на порту 9997. В логах должна присутствовать строка с указанным IMSI.

#### 1.5. *HandlesEmptyImsi*

Тест проверяет, как клиент обрабатывает пустой IMSI. Ожидается, что в логах появится строка "Sending IMSI: ", отражающая отправку пустого значения.

#### 1.6. *EncodesImsiWithOddLength*

Тест проверяет корректную обработку IMSI нечетной длины (12345) при отправке на сервер на порту 9996. В логах должна быть строка "Received response", подтверждающая успешный ответ.

#### 1.7. *SendsToCorrectPort*

Тест удостоверяет, что клиент отправляет IMSI (987654321012345) на правильный порт (9995) и получает ответ. В логах проверяется наличие строки "Received response".

#### 1.8. *HandlesAllZeroImsi*

Тест проверяет отправку IMSI, состоящего из всех нулей (000000000000000), на сервер на порту 9994. В логах ожидается строка "Received response".

#### 1.9. *HandlesAllNineImsi*

Тест аналогично проверяет отправку IMSI из всех девяток (999999999999999) на сервер на порту 9993, с подтверждением ответа в логах строкой "Received response".

#### 1.10. *FailsOnSocketCreation*

Тест имитирует сбой при создании сокета и проверяет, что клиент записывает в лог сообщение "Failed to create socket" при попытке отправки IMSI.

#### 1.11. *FailsOnSendto*

Тест симулирует сбой при вызове функции sendto, проверяя, что в логах появляется сообщение "Failed to send IMSI".

#### 1.12. *FailsOnSelect*

Тест моделирует ошибку при вызове функции select, проверяя, что в логах фиксируется сообщение "Select error on attempt".

#### 1.13. *FailsOnRecvfrom*

Тест имитирует сбой при получении ответа (recvfrom), проверяя, что в логах появляется сообщение "Max retries reached" после исчерпания попыток.

#### 1.14. *HandlesMaxValidImsiLength*

Тест подтверждает, что клиент корректно обрабатывает IMSI максимальной длины (15 цифр, 123456789012345), отправляя его на сервер на порту 9992. В логах ожидается "Received response".

#### 1.15. *HandlesShortImsiLength*

Тест проверяет обработку короткого IMSI (12) при отправке на сервер на порту 9991, с подтверждением ответа в логах строкой "Received response".

#### 1.16. *HandlesSingleDigitImsi*

Тест удостоверяет, что клиент отправляет однозначный IMSI на сервер на порту 9990 и получает ответ, что подтверждается строкой "Received response" в логах.

#### 1.17. *SendImsiTwiceWithSameClient*

Тест проверяет, что один и тот же клиент может отправить два разных IMSI (123456789012345 и 543210987654321) на сервер на порту 9989. В логах должны присутствовать оба IMSI.

#### 1.18. *SendImsiTwiceWithDifferentClients*

Тест подтверждает, что два разных клиента могут отправить разные IMSI (111111111111111 и 222222222222222) на сервер на порту 9988. Оба IMSI должны быть в логах.

#### 1.19. *InfoLevelFiltersOutDebugLogs*

Тест удостоверяет, что при установленном уровне логов "info" отладочные сообщения (debug) не записываются, но отправленный IMSI (333333333333333) на сервер на порту 9987 фиксируется в логах.

#### 1.20. *FailsOnInvalidServerIP*

Тест проверяет, что при использовании невалидного IP-адреса (invalid_ip) клиент записывает в лог сообщение "Invalid server IP address".

#### 1.21. *WarnsOnNoResponseAndExhaustsRetries*

Тест проверяет, что при отсутствии ответа от сервера на порту 12345 и исчерпании попыток в логах появляются сообщения "Timeout on attempt" и "Max retries reached, giving up".

### 2. test_bcd_encoder.cpp

Тестирует класс BcdEncoder, который кодирует строки IMSI в формат Binary-Coded Decimal (BCD). Тесты проверяют корректность кодирования для различных входных данных.

#### 2.1. *EncodesFull15DigitIMSI*

Тест проверяет, что 15-значный IMSI (123456789012345) кодируется в ожидаемую последовательность байтов {0x21, 0x43, 0x65, 0x87, 0x09, 0x21, 0x43, 0xF5}.

#### 2.2. *EncodesEvenLengthIMSI*

Тест удостоверяет, что IMSI четной длины (123456) кодируется в {0x21, 0x43, 0x65}.

#### 2.3. *EncodesOddLengthIMSI*

Тест подтверждает, что IMSI нечетной длины (12345) кодируется как {0x21, 0x43, 0xF5}, с правильной обработкой заполнения.

#### 2.4. *EncodesEmptyString*

Тест проверяет, что пустая строка IMSI возвращает пустой вектор.

#### 2.5. *EncodesSingleDigit*

Тест удостоверяет, что однозначный IMSI (7) кодируется как {0xF7}.

#### 2.6. *EncodesRepeatedDigits*

Тест проверяет кодирование IMSI с повторяющимися цифрами (111111) в {0x11, 0x11, 0x11}.

#### 2.7. *EncodesZerosAndNines*

Тест подтверждает, что IMSI с чередующимися нулями и девятками (090909) кодируется как {0x90, 0x90, 0x90}.

#### 2.8. *EncodesMaxLengthIMSI*

Тест удостоверяет, что 15-значный IMSI из всех единиц (111111111111111) кодируется в 8 байт, с последним байтом 0xF1.

#### 2.9. *Encodes14DigitIMSI*

Тест проверяет, что 14-значный IMSI (12345678901234) кодируется в {0x21, 0x43, 0x65, 0x87, 0x09, 0x21, 0x43}, без заполнения.

#### 2.10. *EncodeNonDigitCharacters_NoCheck*

Тест проверяет поведение при наличии нецифрового символа в IMSI (12A456), ожидая, что выходной вектор имеет длину 3 байта.

### 3. test_logger_initializer_client.cpp

Тестирует класс LoggerInitializer, который настраивает логирование для клиента с использованием библиотеки spdlog. Тесты проверяют создание лог-файлов, запись сообщений и настройку уровней логирования.

#### 3.1. *CreatesLogFile*

Тест проверяет, что при инициализации логгера с валидной конфигурацией создается указанный лог-файл (test_client.log).

#### 3.2. *WritesMessageToLogFile*

Тест удостоверяет, что сообщение ("Logger writes this") записывается в лог-файл при уровне логирования info.

#### 3.3. *LogsAtInfoLevel*

Тест подтверждает, что установка уровня логирования "info" корректно настраивает логгер на уровень info.

#### 3.4. *LogsAtDebugLevel*

Тест проверяет, что уровень логирования "debug" устанавливает логгер на уровень debug.

#### 3.5. *LogsAtWarningLevel*

Тест удостоверяет, что уровень логирования "warning" настраивает логгер на уровень warning.

#### 3.6. *LogsAtErrorLevel*

Тест подтверждает, что уровень логирования "error" устанавливает логгер на уровень error.

#### 3.7. *AllowsReinitialization*

Тест проверяет, что повторная инициализация логгера с той же конфигурацией не вызывает исключений.

#### 3.8. *FlushesOnInfoLevel*

Проверяет, что сообщение на уровне info записывается немедленно после вызова flush/shutdown.

#### 3.9. *LogPatternIncludesTimestampAndLevel*

Тест проверяет, что сообщения лога содержат временную метку (начинающуюся с "[20") и уровень "[info]".

#### 3.10 *InvalidLogLevelThrows*

Тест подтверждает, что невалидный уровень логирования ("no_such_level") вызывает исключение spdlog_ex.

#### 3.11 *DefaultLoggerHasConsoleSink*

Тест удостоверяет, что логгер по умолчанию включает консольный sink (stdout_color_sink_mt).

#### 3.12 *FileSinkRemainsAfterReinit*

Тест проверяет, что повторная инициализация логгера не перезаписывает файловый sink, и в лог-файле присутствуют сообщения "First init" и "Second init".

#### 3.13 *InvalidLogFilePathThrows*

Тест подтверждает, что невалидный путь к лог-файлу (/nonexistent/path/log.txt) вызывает исключение spdlog_ex.

#### 3.14 *DoesNotLogBelowSetLevel*

Тест удостоверяет, что сообщения ниже установленного уровня логирования (например, info при уровне warning) не записываются, а сообщения на уровне warning и выше записываются.

#### 3.15 *SupportsCriticalLogLevel*

Тест проверяет, что уровень логирования "critical" поддерживается и корректно устанавливается.

## *Сервер*

### 1. test_logger_initializer_server.cpp
Этот файл содержит юнит-тесты для класса `LoggerInitializer`, который настраивает логирование для серверной части приложения с использованием библиотеки `spdlog`. Тесты проверяют создание лог-файлов, настройку уровней логирования и обработку ошибок.

#### 1.1 *CreatesLogFile*

Тест проверяет, что при инициализации логгера с валидной конфигурацией (лог-файл test_logger.log и уровень info) создается указанный лог-файл.

#### 1.2 *CreatesLogsDirectory*
 
Тест удостоверяет, что если лог-файл указан в директории ../logs, которая не существует, логгер создает эту директорию при инициализации с уровнем логирования debug.

#### 1.3 *SetsValidLogLevel*
 
Тест подтверждает, что установка уровня логирования warn корректно настраивает логгер на уровень warn.

#### 1.4 *DefaultsToInfoOnInvalidLevel*
 
Тест проверяет, что при указании невалидного уровня логирования (not_a_level) логгер по умолчанию устанавливает уровень info.

#### 1.5 *ThrowsIfFileIsUnwritable*

Тест удостоверяет, что попытка инициализации логгера с недоступным для записи файлом (/root/forbidden.log) вызывает исключение `std::runtime_error`.

#### 1.6 *HandlesOffLogLevel*
 
Тест подтверждает, что уровень логирования off корректно устанавливается, отключая все логирование.

#### 1.7 *HandlesEmptyLogFilePath*

Тест проверяет, что попытка инициализации логгера с пустым путем к лог-файлу вызывает исключение `std::runtime_error`.

#### 1.8 *SupportsTraceLogLevel*

Тест удостоверяет, что уровень логирования trace поддерживается и корректно устанавливается.

#### 1.9 *DoesNotThrowIfLogsDirectoryExists*

Тест подтверждает, что если директория ../logs уже существует, инициализация логгера с файлом в этой директории (../logs/test2.log) не вызывает исключений.

#### 1.10 *MultipleInitializationsAppendSink*
 
Тест проверяет, что многократная инициализация логгера с одной и той же конфигурацией (лог-файл test_logger.log, уровень info) не приводит к ошибкам и корректно добавляет sink.


### 2. test_http_api.cpp
Этот файл тестирует класс `HttpServer`, который реализует HTTP API для проверки статуса подписчиков и управления сервером. Тесты проверяют обработку запросов и поведение сервера.

#### 2.1 *CheckSubscriberNotActive*
 
Тест проверяет, что GET-запрос на /check_subscriber с несуществующим IMSI (00000) возвращает статус 200 и тело ответа "not active\n".

#### 2.2 *CheckSubscriberWithEmptyIMSI*

Тест удостоверяет, что GET-запрос на /check_subscriber с пустым IMSI возвращает статус 200 и тело ответа "not active\n".

#### 2.3 *CheckSubscriberCaseSensitiveIMSI*

Тест подтверждает, что проверка IMSI чувствительна к регистру: IMSI ABC123 добавляется в сессии, но запрос с imsi=abc123 возвращает статус 200 и "not active\n", так как регистр не совпадает.

#### 2.4 *CheckSubscriberKnownIMSI*
 
Тест проверяет, что GET-запрос на /check_subscriber с известным IMSI (99999), добавленным в сессии, возвращает статус 200 и тело ответа "active\n".

#### 2.5 *StopEndpointShutsDownServer*

Тест удостоверяет, что POST-запрос на /stop возвращает статус 200 и тело "stopping\n", после чего сервер останавливается, и последующий GET-запрос на /check_subscriber не возвращает статус 200.

#### 2.6 *CheckSubscriberMissingParam*

Тест подтверждает, что GET-запрос на /check_subscriber без параметра IMSI возвращает статус 400 (ошибка запроса).

### 3. test_decode_utils.cpp

Этот файл тестирует класс `BcdDecoder`, который декодирует Binary-Coded Decimal (BCD) данные в строковый формат IMSI. Тесты проверяют корректность декодирования для различных входных данных.

#### 3.1 *ValidIMSI*

Тест проверяет, что BCD-последовательность {0x21, 0x43, 0x65, 0x87} декодируется в строку IMSI "12345678".

#### 3.2 *IgnoreInvalidDigits*
 
Тест удостоверяет, что при наличии недопустимых цифр в BCD (например, {0x2F, 0x4A}) декодер игнорирует их и возвращает строку "24".

#### 3.3 *EmptyVectorReturnsEmptyString*
  
Тест подтверждает, что пустой вектор BCD возвращает пустую строку.

#### 3.4 *AllInvalidDigits*
 
Тест проверяет, что если все цифры в BCD недопустимы ({0xFA, 0xFE}), декодер возвращает пустую строку.

#### 3.5 *OddDigitsOnly*

Тест удостоверяет, что BCD-последовательность {0x13, 0x57} декодируется в строку "3175", обрабатывая только нечетные позиции.

#### 3.6 *EvenDigitsOnly*
  
Тест подтверждает, что BCD-последовательность {0x20, 0x64} декодируется в строку "0246", обрабатывая четные позиции.

#### 3.7 *HighNibbleOnlyValid*

Тест проверяет, что при валидных старших полубайтах в BCD ({0xF1, 0xF2}) декодер возвращает строку "12".

#### 3.8 *LowNibbleOnlyValid*
 
Тест удостоверяет, что при валидных младших полубайтах в BCD ({0x1F, 0x2F}) декодер возвращает строку "12".

#### 3.9 *LeadingZeroPreserved*
 
Тест подтверждает, что ведущие нули сохраняются при декодировании BCD-последовательности {0x10, 0x02}, возвращая строку "0120".

#### 3.10 *LongIMSI*

Тест проверяет, что длинная BCD-последовательность {0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56} декодируется в строку "2143658709214365".


### 4. test_signal_handler.cpp
Этот файл тестирует класс `SignalHandler`, который управляет обработкой сигналов (например, SIGINT) для корректного завершения работы сервера.

#### 4.1 *RegistersSigintHandler*
 
Тест проверяет, что при создании объекта `SignalHandler` он корректно регистрирует обработчик для SIGINT, сравнивая указатель на текущий обработчик с `SignalHandler::handle`.

#### 4.2 *HandlesUnknownSignal_NoExit*

Тест удостоверяет, что обработка неизвестного сигнала (SIGTERM) не вызывает завершение программы, и программа завершается с кодом 123, как указано в тесте.

#### 4.3 *ExitsOnSigint*

Тест подтверждает, что обработка сигнала SIGINT вызывает завершение программы с кодом выхода 1.

#### 4.4 *MultipleRegistrationsAreIdempotent*

Тест проверяет, что многократная регистрация обработчиков сигналов (создание нескольких объектов `SignalHandler`) не вызывает ошибок.

#### 4.5 *IgnoresZeroSignal*
 
Тест удостоверяет, что сигнал с номером 0 игнорируется, и программа завершается с кодом 42, как указано в тесте.

#### 4.6 *HandlesSigusr1AsUnknown*
  
Тест подтверждает, что сигнал SIGUSR1 обрабатывается как неизвестный, и программа завершается с кодом 99, как указано в тесте.

#### 4.7 *RegistersCorrectFunctionPointer*

Тест проверяет, что регистрация обработчика для SIGINT устанавливает корректный указатель на функцию `SignalHandler::handle`.

#### 4.8 *HandleDoesNotThrow*
  
Тест удостоверяет, что обработка сигнала SIGTERM не вызывает исключений.

#### 4.9 *RegisterThenTrigger*
 
Тест подтверждает, что после регистрации обработчика вызов SIGINT через `raise` завершает программу с кодом выхода 1.

### 5. test_pgw_server.cpp

Тестирует класс `PgwServer`, который реализует серверную логику для обработки UDP-запросов с IMSI и управления сессиями. Тесты проверяют создание сокетов, обработку клиентов, очистку сессий и устойчивость при ошибках.

#### 5.1 *SetupSocketCreatesSocket*
 
Тест проверяет, что метод `test_setup_socket` создает валидный сокет, возвращая дескриптор >= 0.

#### 5.2 *SetupEpollCreatesEpollFD*
 
Тест удостоверяет, что метод `test_setup_epoll` создает валидный epoll-дескриптор для переданного сокета.

#### 5.3 *HandleClient_BlacklistedImsiGetsRejected*
 
Тест подтверждает, что запрос с IMSI из черного списка (1111) обрабатывается сервером, но сессия не создается.

#### 5.4 *HandleClient_NewImsiCreatesSession*
  
Тест проверяет, что запрос с новым IMSI (2143) создает новую сессию на сервере.

#### 5.5 *HandleClient_RepeatedImsiSessionNotDuplicated*
 
Тест удостоверяет, что повторный запрос с тем же IMSI (2143) не создает дублирующую сессию.

#### 5.6 *HandleClient_EmptyImsi*

Тест подтверждает, что пустой IMSI в запросе обрабатывается без создания сессии.

#### 5.7 *SessionCleanerRemovesExpiredSessions*
 
Тест проверяет, что очиститель сессий удаляет истекшие сессии (IMSI 9999 с таймаутом 5 секунд) после выполнения очистки.

#### 5.8 *SessionCleanerDoesNotRemoveActiveSessions*
 
Тест удостоверяет, что активные сессии (IMSI 8888, созданные недавно) не удаляются очистителем.

#### 5.9 *SetupSocketFailsOnInvalidPort*
 
Тест подтверждает, что создание сокета с невалидным портом (65536) завершается ошибкой, возвращая -1.

#### 5.10 *SetupSocketFailsOnPortInUse*
 
Тест проверяет, что попытка создать сокет на уже занятом порте (9999) возвращает -1.

#### 5.11 *HandleClient_InvalidBuffer*

Тест удостоверяет, что запрос с невалидным BCD-буфером ({0xFF, 0xFF}) не создает сессию для IMSI "FFFF".

#### 5.12 *HandleClient_LargeBufferRejectedDueToInvalidImsi*
  
Тест подтверждает, что запрос с большим буфером (1024 байта) отклоняется, и сессия не создается.

#### 5.13 *SessionCleanerHandlesMultipleSessions*

Тест проверяет, что очиститель сессий корректно удаляет истекшие сессии (1111, 3333) и сохраняет активные (2222, 4444).

#### 5.14 *SessionCleanerEmptyTable*
 
Тест удостоверяет, что очиститель сессий корректно работает с пустой таблицей сессий, не создавая ошибок.

#### 5.15 *RunHandlesMultipleClients*
 
Тест подтверждает, что сервер обрабатывает несколько IMSI-запросов (123456789012345, 432176549876543 и др.), создавая сессии для каждого IMSI, и останавливается через POST-запрос на /stop.

#### 5.16 *HandleClient_ResponseReceived*

Тест проверяет, что сервер отвечает строкой "created" на запрос с IMSI 123456789012345, отправленный по UDP.

#### 5.17 *RunHandlesEpollWaitEINTR*

Тест удостоверяет, что сервер корректно обрабатывает прерывание epoll_wait сигналом SIGUSR1 и продолжает работу до остановки через /stop.

#### 5.18 *RunHandlesThousandClientsConcurrently*
 
Тест подтверждает, что сервер обрабатывает 20000 одновременных IMSI-запросов, создавая сессии для каждого IMSI, и корректно останавливается.

#### 5.19 *DestructorJoinsThread*
  
Тест проверяет, что деструктор `PgwServer` корректно завершает потоки, добавленные в пул, без сбоев.

#### 5.20 *SocketCreationFails*

Тест удостоверяет, что создание сокета с невалидным IP-адресом (999.999.999.999) возвращает -1.

#### 5.21 *SetupEpollFailsOnBadFD*
 
Тест подтверждает, что создание epoll-дескриптора с невалидным сокетом (-1) возвращает -1.

#### 5.22 *RunHandlesEpollWaitFatalError*
 
Тест проверяет, что сервер корректно завершает работу при фатальной ошибке epoll_wait, вызванной, например, закрытием дескриптора.

#### 5.23 *SetupSocketFailsOnTooManyFDs*
  
Тест удостоверяет, что создание сокета при исчерпании файловых дескрипторов (после открытия 1024 файлов) возвращает -1.

#### 5.24 *SetupEpollFailsAfterSockClose*
 
Тест подтверждает, что попытка создания epoll-дескриптора для закрытого сокета возвращает -1.

#### 5.25 *HandleClient_BlacklistedImsi_RespondsRejected*
 
Тест проверяет, что запрос с IMSI из черного списка (123456789012345) получает ответ "rejected" от сервера.

### 6. test_cdr_writer.cpp
Этот файл тестирует класс `Ccdr_writer`, который записывает CDR (Call Detail Record) в лог-файл. Тесты проверяют запись данных, обработку ошибок и формат временных меток.

#### 6.1 *WriteCDRFile*
 
Тест проверяет, что запись IMSI (123456789012345) и статуса "create" создает лог-файл test_cdr.log, содержащий указанные данные.

#### 6.2 *WriteEmptyValues*
 
Тест удостоверяет, что запись пустых значений IMSI и статуса создает лог-файл test_cdr_empty.log с пустыми полями, разделенными запятыми.

#### 6.3 *HandleFileOpenFailure*

Тест подтверждает, что попытка записи в недоступный файл (/nonexistent_dir/test_cdr.log) не вызывает исключений.

#### 6.4 *MultipleWritesAppend*
 
Тест проверяет, что многократные записи (IMSI1 с "create" и IMSI2 с "timeout") добавляются в лог-файл test_cdr_append.log последовательно.

#### 6.5 *TimestampFormatCheck*

Тест удостоверяет, что запись в лог-файл test_cdr_time.log содержит временную метку в формате "YYYY-MM-DD HH:MM:SS", проверяемую через регулярное выражение.

#### 6.6 *WriteFailureAfterOpen*

Тест подтверждает, что попытка записи в файл с правами только на чтение (readonly_cdr.log) не приводит к сбою, хотя запись не выполняется.

# Как собрать?
```bash
# Сборка в Release
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release -- -j$(nproc)

# Сборка в Debug
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug -- -j$(nproc)
```

# Как использовать?

## *Клиент*
```bash
./build/pgw_client <IMSI>
```
## *Сервер*
```bash
./build/pgw_server
```
### Проверка статуса абонента (```/check_subscriber```)
```bash
curl "http://localhost:<http_port>/check_subscriber?imsi=<number>"
```
### Остановка сервера (```/stop```)
```bash
curl -X POST http://localhost:<http_port>/stop
```
Для изменения параметров сервера и клиента использовать файлы конфигурации в папке ```/config```
# Покрытие кода

## *Как измерить покрытие кода?*

```bash
# тестировалось в Ubuntu 24.04 LTS
# в корне проекта 
sudo apt update
sudo apt install lcov
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build -- -j$(nproc)
ctest --test-dir build
cmake --build build --target coverage
xdg-open build/coverage-report/index.html
```

## *Результаты*
```bash
Overall coverage rate:
  lines......: 95.2% (394 of 414 lines)
  functions......: 100.0% (41 of 41 functions)
[100%] Built target coverage
```
