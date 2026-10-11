# Reset aleatório da ESP32-S3 — correção DabbleESP32

> **IMPORTANTE:** O firmware `.ino` NÃO contém esta correção. O problema foi observado em uma versão da biblioteca **DabbleESP32**, que precisa ser corrigida no computador antes de recompilar o jogo. O autor aplicou o patch e confirmou que os resets que observava cessaram.

## Erro observado

```text
assert failed: heap_caps_free heap_caps_base.c:80
(heap != NULL && "free() target pointer is outside heap areas")
ELF file SHA256: bf3df46c9
rst:0xc (RTC_SW_CPU_RST)
```

O ELF de `sketch_oct10a.ino` com prefixo SHA256 `bf3df46c9` permitiu decodificar o backtrace:

| Etapa | Função |
| --- | --- |
| `DabbleESP32.cpp:517` | `DabbleClass::freeMemoryAllocated()` — `delete[] arguments[i]` |
| `DabbleESP32.cpp:332` | `DabbleClass::processInput(int)` |
| `DabbleESP32.cpp:495` | `DabbleClass::processInput()` |
| `sketch_oct10a.ino:833` | `pollDabble()` |
| `sketch_oct10a.ino:3324` | `loop()` |

**Confirmado:** nesta falha houve liberação de ponteiro inválido durante o processamento BLE; não se tratou de um brownout indicado pelo log. A [biblioteca pública da STEMpedia](https://github.com/STEMpedia/DabbleESP32/blob/master/src/DabbleESP32.cpp) cria o vetor `arguments` sem inicializá-lo, mas tenta liberar todas as posições ao encerrar um quadro. Quando um pacote é interrompido, isso pode levar a `delete[]` sobre um ponteiro não inicializado. A hipótese é compatível com o backtrace, embora corrupção anterior não possa ser totalmente excluída.

## Patch local

Trocar na biblioteca `DabbleESP32.cpp`:

```cpp
// Antes:
arguments = new byte* [argumentnumber];
argumentL = new byte [argumentnumber];
// Depois:
arguments = new byte*[argumentnumber]();
argumentL = new byte[argumentnumber]();
```

Os parênteses inicializam ponteiros em `nullptr` e comprimentos em zero; `delete[] nullptr` é permitido em C++. **O autor confirmou a solução na placa**, mas outros tipos de reset ainda podem existir.

## Aplicar no Windows

1. **Feche o Arduino IDE**.
2. Baixe [tools/CORRIGIR-DABBLE-ESP32S3.ps1](../tools/CORRIGIR-DABBLE-ESP32S3.ps1) na pasta **Downloads**.
3. Pressione `Win + E`, abra Downloads, clique na **barra de endereços**, digite `powershell` e pressione Enter.
4. Execute:

```powershell
powershell -ExecutionPolicy Bypass -File .\CORRIGIR-DABBLE-ESP32S3.ps1
```

5. Aguarde a mensagem `DabbleESP32 corrigido` e guarde o caminho indicado em `Backup`. Se a biblioteca estiver em outra pasta, informe `-LibraryFile 'C:\CAMINHO\DabbleESP32\src\DabbleESP32.cpp'`.
6. Reabra o Arduino IDE, compile e grave o jogo novamente. Teste partidas longas e reconexões do controle com o Monitor Serial em 115200 baud.

**Reversão:** feche o Arduino IDE; na pasta `DabbleESP32\src`, restaure o arquivo `.bak-AAAAmmdd-HHMMss` renomeando-o para `DabbleESP32.cpp`, e recompile. O script não precisa de permissão de administrador e aborta se não reconhecer as linhas esperadas.

**Atenção:** reinstalar ou atualizar DabbleESP32 pode remover esse patch. Se voltar a ocorrer reset, guarde o backtrace completo e o arquivo `.elf` da **mesma compilação** para diagnóstico.
