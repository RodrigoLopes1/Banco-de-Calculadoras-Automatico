function doGet(e) { 
  Logger.log( JSON.stringify(e) );
  var resultado = 'Ok';
  if (e.parameter == 'undefined') {
    resultado = 'Nenhum parâmetro';
  }
  else {
    var planilhaID = '1skj_xNlBYRW0fGmlolrgmIFdTrFOagWfHlKMhOr_bi4'; 	// Spreadsheet ID
    var planilha = SpreadsheetApp.openById(planilhaID).getActiveSheet();
    var newRow = planilha.getLastRow() + 1;						
    var linhaDados = [];


    var dtaAtual = new Date();
    linhaDados[0] = dtaAtual; // Date in column A
    var horaAtual = Utilities.formatDate(dtaAtual, 'America/Sao_Paulo', 'HH:mm:ss');
    //Utilities.formatDate(Curr_Date, 'America/Sao_Paulo', 'HH:mm:ss');
    //https://stackoverflow.com/questions/18596933/google-apps-script-formatdate-using-users-time-zone-instead-of-gmt
    //https://joda-time.sourceforge.net/timezones.html


    linhaDados[1] = horaAtual; // Time in column B


    for (var param in e.parameter) {
      Logger.log('In for loop, param=' + param);
      var valor = stripQuotes(e.parameter[param]);
      Logger.log(param + ':' + e.parameter[param]);
      switch (param) {
        case 'nome':
          linhaDados[2] = valor; // Temperature in column C
          resultado = 'Descrição foi inserido na coluna C'; 
          break;
        case 'valor':
          linhaDados[3] = valor; // Humidity in column D
          resultado += ' , Valor foi inserido na coluna D'; 
          break;  
        default:
          resultado = "parametro nao suportado. Entre em contato com o desenvolvedor";
      }
    }
    Logger.log(JSON.stringify(linhaDados));
    var newRange = planilha.getRange(newRow, 1, 1, linhaDados.length);
    newRange.setValues([linhaDados]);
  }
  return ContentService.createTextOutput(resultado);
}
function stripQuotes( value ) {
  return value.replace(/^["']|['"]$/g, "");
}
