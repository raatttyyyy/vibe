# Vibe: интерфейсы между модулями

Документ описывает, чем модули транслятора обмениваются друг с другом: лексемы, дерево разбора, результат семантического анализа, сообщения об ошибках. Требования, которые эти интерфейсы обслуживают, — в [SPEC.md](SPEC.md).

**Статус:** черновик контракта, версия 0.1 от 10.11.2026. Кода ещё нет; объявления ниже — предложение, которое уточняется вместе с реализацией. Часть про генерацию и исполнение намеренно краткая.

## 1. Конвейер

```
текст ──► лексер ──► лексемы ──► парсер ──► дерево ──► семантика ──► дерево с типами ──► генерация ──► C++
                                                                      + таблицы имён
```

```cpp
namespace vibe {

std::vector<Token>    tokenize(std::string_view source, DiagnosticSink& diags);
std::unique_ptr<Node> parse(const std::vector<Token>& tokens, DiagnosticSink& diags);
SemaResult            analyze(Node& program, DiagnosticSink& diags);
std::string           generateCpp(const Node& program, const SemaResult& sema, DiagnosticSink& diags);

}
```

Правила:

- Следующая функция вызывается, только если `diags.hasErrors()` ложно.
- Модули не обращаются к внутренностям друг друга: только к типам из этого документа.
- Всё лежит в пространстве имён `vibe`.

## 2. Позиции в тексте

```cpp
struct SourcePos {
    std::uint32_t line   = 1;  // с единицы
    std::uint32_t column = 1;  // с единицы, в символах, а не в байтах
    std::uint32_t offset = 0;  // смещение в байтах от начала файла
};

struct SourceRange {
    SourcePos begin;  // первый символ
    SourcePos end;    // позиция сразу за последним символом
};
```

## 3. Лексемы (лексер → парсер)

```cpp
enum class TokenType {
    Keyword,     // ключевое слово, найдено бором
    Operator,    // + - * / % = == != < <= > >= && || ! и другие по грамматике
    Literal,     // число, символ, строка, промпт
    Identifier,  // id
    Delimiter,   // ( ) { } [ ] ; , : .
    Unknown,     // лексическая ошибка
    EndOfFile    // служебная, всегда последняя
};

enum class LiteralKind { None, Int, Double, Char, String, Prompt };

struct Token {
    TokenType   type;
    LiteralKind literal = LiteralKind::None;  // заполнено только у Literal
    std::string lexeme;                       // точный текст из исходника
    SourceRange range;
};
```

Договорённости:

- Первые шесть типов — список с доски; `EndOfFile` добавлен, чтобы парсер мог проверить конец разбора (SYN-4).
- Последовательность всегда заканчивается ровно одной лексемой `EndOfFile`.
- Пробельные символы и комментарии в последовательность не попадают.
- `func` распознаётся как `Keyword`. Имена `main` и `funcName` распознаются как `Identifier`.
- `true` и `false` — ключевые слова; литералами их считает парсер.
- Какая именно операция или ключевое слово перед парсером, он определяет по `lexeme`.
- На каждую лексему `Unknown` лексер выдаёт сообщение об ошибке.
- К какому типу отнести запятую — `Operator` или `Delimiter` — решает грамматика: по К-3 запятая является операцией.
- Виды литералов дополняются по грамматике (например, величины с единицами в бюджетах).

## 4. Сообщения об ошибках (все модули)

```cpp
enum class Severity { Error, Warning, Note };
enum class Stage    { Lexer, Parser, Sema, Codegen };

struct Diagnostic {
    Severity    severity;
    Stage       stage;
    std::string code;     // "LEX001", "SYN014", "SEM003"
    std::string message;
    SourceRange range;
};

class DiagnosticSink {
public:
    void report(Diagnostic d);
    bool hasErrors() const;
    const std::vector<Diagnostic>& all() const;
};
```

Формат вывода:

```
main.vibe:3:14: ошибка LEX001: недопустимый символ '#'
```

Коды состоят из префикса этапа (`LEX`, `SYN`, `SEM`, `GEN`) и номера. Список кодов каждого этапа ведётся в документации модуля.

## 5. Дерево разбора (парсер → семантика → генерация)

Узел один для всех конструкций, как `Node` в конспекте, но с произвольным числом потомков.

```cpp
struct Node {
    NodeKind                           kind;
    Token                              token;     // операция, имя или литерал; см. таблицу
    std::vector<std::unique_ptr<Node>> children;  // порядок задан таблицей
    SourceRange                        range;     // вся конструкция целиком

    // заполняет семантический анализ
    const Type*   type   = nullptr;  // у выражений
    const Symbol* symbol = nullptr;  // у имён и объявлений
};
```

Отсутствующий необязательный потомок — `nullptr` на своём месте, а не пропуск: у `If` без `else` три потомка, третий пуст.

Виды узлов классической части:

| `NodeKind` | `token` | Потомки по порядку |
|---|---|---|
| `Program` | — | определения и объявления |
| `FuncDef` | имя после `func` и типа результата | тип результата, `Params`, `Block` |
| `Params` | — | `Param`… |
| `Param` | имя | тип |
| `StructDef` | имя | `Field`… |
| `Field` | имя | тип |
| `EnumDef` | имя | `EnumItem`… |
| `EnumItem` | имя | — |
| `VarDecl` | имя | тип, начальное значение или пусто |
| `TypeRef` | имя типа | аргументы типа |
| `Block` | — | операторы |
| `If` | `if` | условие, ветка «то», ветка «иначе» или пусто |
| `While` | `while` | условие, тело |
| `DoWhile` | `do` | тело, условие |
| `For` | `for` | начало или пусто, условие или пусто, шаг или пусто, тело |
| `Break`, `Continue` | слово | — |
| `Return` | `return` | значение или пусто |
| `Goto` | имя метки | — |
| `Label` | имя метки | помеченный оператор |
| `ExprStmt` | — | выражение |
| `Assign` | `=` | куда, что |
| `Binary` | операция | левый, правый |
| `Unary` | операция | операнд |
| `Comma` | `,` | левый, правый |
| `Call` | — | вызываемое, аргументы… |
| `Index` | — | контейнер, индекс |
| `Member` | имя поля | объект |
| `Name` | имя | — |
| `Literal` | литерал | — |

Для агентной части зарезервированы виды `ModelDef`, `AgentDef`, `TaskDef`, `Handler`, `Context`, `Use`, `Hide`, `Attempt`, `Ensure`, `Spawn`, `Await`, `Prompt`. Состав их потомков фиксируется после утверждения грамматики.

Таблица следует за грамматикой: при изменении правила меняется и строка здесь.

## 6. Результат семантического анализа (семантика → генерация)

```cpp
enum class TypeKind {
    Void, Int, Double, Bool, Char, String,
    List, Map, Struct, Enum, Future, Function,
    Error  // тип выражения с ошибкой; не порождает новых сообщений
};

struct Type {
    TypeKind                 kind;
    std::string              name;  // для Struct и Enum
    std::vector<const Type*> args;  // элементы контейнера, параметры и результат функции
};

enum class SymbolKind {
    Variable, Parameter, Function, Struct, Field, Enum, EnumItem, Label,
    Model, Agent, Task
};

struct Symbol {
    SymbolKind  kind;
    std::string name;
    const Type* type;
    SourceRange declared;
};

struct SemaResult {
    // владеет всеми Type и Symbol, на которые ссылаются узлы дерева
};
```

Договорённости:

- После успешного анализа у каждого узла-выражения заполнен `type`, у каждого `Name` и объявления — `symbol`.
- Одинаковые типы представлены одним объектом: сравнение типов — сравнение указателей.
- `SemaResult` живёт не меньше дерева.

## 7. Генерация и исполнение

`generateCpp` возвращает текст одного файла на C++20, который подключает библиотеку времени исполнения. Интерфейс этой библиотеки и то, во что переводится каждая конструкция, описываются в `docs/generation/` и `docs/execution/`.

## 8. Как менять этот документ

- Интерфейс меняется в том же изменении, что и код по обе его стороны.
- Изменение, которое затрагивает чужой модуль, согласуется с его ответственным (таблица в [SPEC.md](SPEC.md), раздел 2).
