#include "Token.h"

std::string tokenTypeToString(TokenType tipo) {
    switch (tipo) {
        case NUM_INT:    return "NUM_INT";
        case NUM_DEC:    return "NUM_DEC";
        case ID:         return "ID";
        case TEXTO:      return "TEXTO";

        case INT:        return "INT";
        case FLOAT:      return "FLOAT";
        case CHAR:       return "CHAR";
        case BOOLEAN:    return "BOOLEAN";
        case VOID:       return "VOID";
        case IF:         return "IF";
        case ELSE:       return "ELSE";
        case FOR:        return "FOR";
        case WHILE:      return "WHILE";
        case SCANF:      return "SCANF";
        case PRINTLN:    return "PRINTLN";
        case MAIN:       return "MAIN";
        case RETURN:     return "RETURN";

        case ASIGNACION: return "=";
        case SUMA:       return "+";
        case RESTA:      return "-";
        case MULT:       return "*";
        case DIV:        return "/";
        case MOD:        return "%";
        case AND:        return "&&";
        case OR:         return "||";
        case NOT:        return "!";
        case COMP:       return "COMP"; 

        case PAR_IZQ:    return "(";
        case PAR_DER:    return ")";
        case CORCH_IZQ:  return "[";
        case CORCH_DER:  return "]";
        case LLAVE_IZQ:  return "{";
        case LLAVE_DER:  return "}";
        case COMA:       return ",";
        case PUNTO_COMA: return ";";
        case PUNTO:      return "."; // <--- CAMBIO REALIZADO: Retorno en texto para PUNTO

        case DESCONOCIDO: return "DESCONOCIDO";
        case FIN_ARCHIVO: return "EOF";
        default:          return "INDEFINIDO";
    }
}