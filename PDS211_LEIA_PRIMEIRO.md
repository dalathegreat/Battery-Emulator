# PDS211: Dala atual recebido + Waveshare LCD-5B

24/10/2026. **Código fonte integrado para compilar no teu PC. Não inclui firmware BIN nem uma compilação ESP32 validada.**

## Base e decisão

Este pacote substitui o PDS209 como candidato para a comparação de três versões.

Base integral: ZIP Dala confirmado pelo utilizador, com identificador interno `7d03ad5d07a5cd8b5fb1a0d6e5b8608b231f5184`. É a versão recebida nesta conversa, não uma afirmação de que corresponde ao HEAD atual online.

Referências adicionais:

- GitHub do utilizador: `5f60d3e6d9ddacd5ef32f9da0921e4adb6d5d279`.
- Pasta VS Code: fonte indicada pelo utilizador como a que usa no equipamento. Sem commit verificável; o binário instalado não foi lido.

Estratégia autorizada: manter o mais próximo possível do Dala. Não usar o núcleo antigo local, não misturar bibliotecas CAN antigas nem trazer dependências e ficheiros de compilação gerados.

## Resultado exato

Projeto com **560 ficheiros**:

- **548 dos 553 ficheiros Dala permanecem byte a byte idênticos.**
- **5 ficheiros Dala modificados.**
- **7 ficheiros personalizados adicionados.**
- Nenhum ficheiro Dala eliminado.

Os cinco modificados:

1. `Software/Software.cpp`: reconhecer LCD-5B; inclusão/inicialização da bomba apenas nessa placa; espera USB limitada; stack de conectividade de 16384 bytes; não configurar GPIO0 como botão.
2. `Software/src/devboard/hal/hal.cpp`: seleção de `WaveshareLCD5BHal`.
3. `Software/src/devboard/display/display.cpp`: excluir OLED genérico na placa LCD-5B.
4. `platformio.ini`: acrescentar o ambiente LCD-5B e selecioná-lo por defeito.
5. `.gitignore`: permitir versionar `sdkconfig.waveshare.defaults`.

Os sete adicionados:

- `Software/src/communication/pump/pump_link.cpp`
- `Software/src/communication/pump/pump_link.h`
- `Software/src/devboard/display/display_waveshare_LCD_5B.cpp`
- `Software/src/devboard/display/waveshare_ui_text.h`
- `Software/src/devboard/hal/hw_waveshare_LCD_5B.h`
- `include/lv_conf.h`
- `sdkconfig.waveshare.defaults`

Não modificámos os drivers OEM, protocolos de inversor, segurança, datalayer, NVM, bibliotecas CAN ou testes do Dala. Isso preserva as alterações upstream; não certifica todos esses subsistemas.

Os workflows GitHub também permanecem iguais ao Dala. **Não incluem automaticamente um job LCD-5B**; nesta entrega a compilação dessa placa é local e explícita.

## Adaptações locais preservadas

- Dashboard e touch LCD-5B, 98 células e SOC real da bateria principal.
- Menu SOC com passos de 10 pontos percentuais, preset 10/100 e rescaling enable-only.
- Ajustes de corrente e ligação RS485 à LilyGo, protocolo PB1.
- Correção RX PDS199 e HAL CAN15/16, RS485 TX44/RX43.
- GPIO0 reservado ao barramento RGB.
- Stack LCD de 16384 bytes; stack da tarefa da bomba de 4096 bytes.

Os sete acessos antigos da interface a `battery.settings`, incluindo dois via alias `b.settings`, foram migrados para `datalayer.battery_settings`. A pasta local tinha a estrutura antiga válida; o GitHub já tinha atualizado o núcleo sem atualizar esses acessos.

Não redesenhámos a interface nem a convertimos numa vista agregada multi-bateria. A vista de 98 células e medições continua a referir-se ao pack principal; as definições passaram a ser comuns, seguindo a datalayer atual. A lógica de agregação e de limites para o inversor fica a cargo do núcleo Dala.

## Configuração de compilação

- Ambiente: `waveshare_LCD_5B`.
- Plataforma pioarduino: **55.03.312-1**, igual ao Dala recebido.
- LVGL **9.3.0**, LovyanGFX **1.2.26**.
- Memória `qio_opi`, partições `default_16MB.csv`.
- Configuração SDK LCD preserva a consola USB Serial JTAG e desativa explicitamente `CONFIG_NEWLIB_NANO_FORMAT` para manter o formato decimal `%f` utilizado no ecrã.

Não se inclui o `sdkconfig.waveshare_LCD_5B` gerado na instalação antiga. PlatformIO deve gerar a configuração para o ambiente atual. Também não se incluem `.pio`, `managed_components`, `.dummy`, credenciais/configurações do editor, cabeçalhos gerados ou o patcher antigo de caminhos 55.03.311.

## Como compilar sem tocar no projeto que funciona

1. Mantém intacta a pasta atual do VS Code e guarda o firmware anterior se o tens.
2. Extrai este ZIP para `C:\BE211`. Deve existir:
   `C:\BE211\Battery-Emulator-PDS211\platformio.ini`
3. No VS Code: **File > Open Folder**, abre `C:\BE211\Battery-Emulator-PDS211`.
4. Abre um terminal PowerShell e executa esta linha:

```powershell
$env:PLATFORMIO_PACKAGES_DIR = 'C:\Users\Admin\.platformio\packages-waveshare'; & "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -d 'C:\BE211\Battery-Emulator-PDS211' -e waveshare_LCD_5B 2>&1 | Tee-Object -FilePath 'C:\BE211\build-PDS211.log'
```

Este comando **só compila**, não faz upload, não apaga flash e não grava configurações na placa.

A variável de ambiente aponta para o diretório externo curto que já usavas. O projeto não fica preso a esse caminho Windows: a opção é apenas desta sessão PowerShell. Ferramentas partilhadas nesse diretório podem ser instaladas/atualizadas pelo PlatformIO; a pasta de fontes antiga não é modificada por este comando.

É preciso acesso à Internet no teu PC se faltarem dependências. A instalação PlatformIO deve estar no caminho indicado; se não estiver, envia o erro em vez de reinstalar ou aplicar patches antigos.

**Envia o `build-PDS211.log`, mesmo que termine em SUCCESS.** O checkpoint seguinte é conferir compilação, tamanho da imagem e eventuais avisos, antes de preparar a atualização.

## Verificações executadas aqui

- Comparação integral por caminho/hash contra o Dala: 548 iguais, 5 modificados e 7 adicionados.
- Verificação da alteração exata dos sete acessos da interface.
- Verificação estática independente dos pontos de ligação LCD, GPIO0, stack, HAL, bomba, RS485 e configurações.
- Leitura da configuração PlatformIO e pesquisa de marcadores de conflito.
- Sonda C++20 de sintaxe com o header real da datalayer: **19 referências únicas utilizadas pela interface passaram**.

Esta sonda não compila o LCD/LVGL/LovyanGFX e não é uma compilação ESP32.

**Não executado:** build PlatformIO, ligação final, testes unitários upstream, arranque na placa, ensaios de CAN/RS485/touch, persistência NVM ou OTA. Este ambiente não tem PlatformIO/compilador ESP32 nem acesso à Internet para obter dependências.

## Antes de atualizar a placa

- Regista/exporta as configurações web efetivas: bateria, inversor/protocolo, interfaces, limites de corrente/tensão, capacidade, SOC e rede.
- Preserva o projeto/firmware que funciona e o procedimento de recuperação.
- Não apagues NVS/flash para experimentar. Nomes de chaves SOC/limites mantêm-se na inspeção, mas não testámos a migração completa.
- Programa a interrupção da comunicação bateria/inversor num estado de manutenção apropriado aos equipamentos.
- Não assumas que as portas COM antigas continuam iguais.

Após atualização, validar: ausência de reinícios, heap, CAN KIA/inversor, ecrã/touch/98 células, limites de corrente, SOC real/reescalado e persistência, bomba manual/AUTO e resposta RS485. OTA só depois de estabilizar a versão.

O watchdog da bomba/comando zero não garante paragem física: a perda de comunicação com a LilyGo/CAN pode ativar o fallback da bomba.

## Ficheiros auxiliares no ZIP

- `PDS211_manifest.csv`: caminho, relação com Dala e SHA-256.
- `PDS211_validation.json`: resultados das verificações e validações pendentes.
- `PDS211_changes.patch`: diferenças mínimas contra o Dala recebido, para revisão; não precisas de aplicar o patch, o projeto já está integrado.
- Este PDS.

Nenhum repositório remoto ou equipamento foi alterado. Próximo passo: compilação local e análise do log.
