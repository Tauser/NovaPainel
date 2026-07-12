#pragma once

namespace nova::strings_ptbr {

inline constexpr char kAppTitle[] = "NovaPanel";
inline constexpr char kBootTitle[] = "Inicializando";
inline constexpr char kHomeTitle[] = "Inicio";
inline constexpr char kPlaceholderTitle[] = "Placeholder";
inline constexpr char kSetupTitle[] = "Setup";
inline constexpr char kBootHeadlineReady[] = "Display pronto";
inline constexpr char kBootHeadlineRetry[] = "Tentando iniciar display";
inline constexpr char kBootDetailReady[] = "Entrando na tela inicial";
inline constexpr char kBootDetailRetry[] = "Display em retry com breadcrumb";
inline constexpr char kHomeStatusReady[] = "Display ativo";
inline constexpr char kHomeStatusDegraded[] = "Display degradado";
inline constexpr char kHomeDetailMock[] = "Sem rede, usando baseline local";
inline constexpr char kHomeDetailNetwork[] = "Rede ainda nao configurada";
inline constexpr char kPlaceholderMessage[] = "Tela registrada para navegacao da Fase 2";
inline constexpr char kSetupMessage[] = "Onboarding e rede entram nesta tela na Fase 3";
inline constexpr char kSetupTransportOffline[] = "Transporte ESP-Hosted indisponivel";
inline constexpr char kSetupScanning[] = "Transporte ativo; buscando redes Wi-Fi";
inline constexpr char kSetupConnecting[] = "Credenciais salvas; tentando reconectar Wi-Fi";
inline constexpr char kSetupConnected[] = "Wi-Fi conectado; onboarding pode continuar";
inline constexpr char kSetupScanFailed[] = "Scan Wi-Fi falhou; aguardando novo fluxo do wizard";
inline constexpr char kSetupConnectFailed[] = "Reconexao Wi-Fi esgotada neste boot";
inline constexpr char kSetupTransportReady[] = "Transporte ativo; aguardando wizard/onboarding";
inline constexpr char kSetupStepWifi[] = "Passo 1/3  Escolha a rede Wi-Fi";
inline constexpr char kSetupStepTimezone[] = "Passo 2/3  Fuso horario e formato";
inline constexpr char kSetupStepConfirmation[] = "Passo 3/3  Confirmacao";
inline constexpr char kSetupButtonBack[] = "Voltar";
inline constexpr char kSetupButtonNext[] = "Avancar";
inline constexpr char kSetupButtonFinish[] = "Concluir";
inline constexpr char kSetupButtonRescan[] = "Escanear";
inline constexpr char kSetupEmptyNetworks[] = "Nenhuma rede encontrada ainda";
inline constexpr char kSetupWifiSelectedPrefix[] = "Rede selecionada:";
inline constexpr char kSetupWifiStatusOpen[] = "Rede aberta pronta para conectar";
inline constexpr char kSetupWifiStatusPasswordPending[] = "Rede protegida exige teclado compartilhado para senha";
inline constexpr char kSetupWifiStatusNotSelected[] = "Selecione uma rede para continuar";
inline constexpr char kSetupTimezoneLabel[] = "Fuso horario";
inline constexpr char kSetupFormatLabel[] = "Formato da hora";
inline constexpr char kSetupFormat24h[] = "24h";
inline constexpr char kSetupFormat12h[] = "12h";
inline constexpr char kSetupButtonChangeTimezone[] = "Trocar fuso";
inline constexpr char kSetupButtonToggleFormat[] = "Alternar formato";
inline constexpr char kSetupConfirmationWifi[] = "Wi-Fi";
inline constexpr char kSetupConfirmationTimezone[] = "Fuso";
inline constexpr char kSetupConfirmationFormat[] = "Hora";
inline constexpr char kSetupConfirmationPending[] = "Pendente";
inline constexpr char kSetupPasswordLabel[] = "Senha da rede";
inline constexpr char kSetupPasswordPlaceholder[] = "Toque para digitar";
inline constexpr char kSetupWifiStatusPasswordReady[] = "Senha pronta; avance para conectar";
inline constexpr char kToastBootRecovered[] = "Display recuperado apos retry";
inline constexpr char kToastShellReady[] = "Shell pronto";
inline constexpr char kKeyboardPlaceholder[] = "Teclado compartilhado pronto";

}  // namespace nova::strings_ptbr
