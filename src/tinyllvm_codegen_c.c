/**
 * ==============================================================================
 * TinyLLVM Compiler - C Code Generator
 * ==============================================================================
 * 
 * Generates C99 code from a typed AST.
 */

#include "include/tinyllvm_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ==============================================================================
 * Code Generation State
 * ==============================================================================
 */

typedef struct {
    char *output;
    size_t length;
    size_t capacity;
    int indent_level;
    
    CompilerConfig *config;
} CodeGen;

static bool codegen_grow(CodeGen *gen, size_t additional) {
    size_t needed = gen->length + additional + 1;
    if (needed <= gen->capacity) return true;
    
    size_t new_capacity = gen->capacity == 0 ? 1024 : gen->capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }
    
    char *new_output = realloc(gen->output, new_capacity);
    if (!new_output) return false;
    
    gen->output = new_output;
    gen->capacity = new_capacity;
    return true;
}

static bool codegen_append(CodeGen *gen, const char *str) {
    size_t len = strlen(str);
    if (!codegen_grow(gen, len)) return false;
    
    memcpy(gen->output + gen->length, str, len);
    gen->length += len;
    gen->output[gen->length] = '\0';
    
    return true;
}

static bool codegen_appendf(CodeGen *gen, const char *format, ...) {
    char buffer[1024];
    
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len < 0 || (size_t)len >= sizeof(buffer)) {
        return false;
    }
    
    return codegen_append(gen, buffer);
}

static bool codegen_indent(CodeGen *gen) {
    for (int i = 0; i < gen->indent_level; i++) {
        if (!codegen_append(gen, "    ")) return false;
    }
    return true;
}

/* ==============================================================================
 * Forward Declarations
 * ==============================================================================
 */

static bool generate_expression(CodeGen *gen, ASTExpr *expr);
static bool generate_statement(CodeGen *gen, ASTStmt *stmt);

/* ==============================================================================
 * C Code Generation - Expressions
 * ==============================================================================
 */

static bool generate_expression(CodeGen *gen, ASTExpr *expr) {
    if (!expr) return false;
    
    switch (expr->kind) {
        case EXPR_INT_LITERAL:
            return codegen_appendf(gen, "%d", expr->data.int_lit.value);
            
        case EXPR_BOOL_LITERAL:
            return codegen_append(gen, expr->data.bool_lit.value ? "1" : "0");
            
        case EXPR_VAR:
            return codegen_append(gen, expr->data.var.name);
            
        case EXPR_ADD:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " + ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_SUB:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " - ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_MUL:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " * ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_DIV:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " / ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_MOD:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " % ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_EQ:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " == ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_NE:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " != ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_LT:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " < ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_LE:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " <= ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_GT:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " > ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_GE:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " >= ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_AND:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " && ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_OR:
            if (!codegen_append(gen, "(")) return false;
            if (!generate_expression(gen, expr->data.binary.left)) return false;
            if (!codegen_append(gen, " || ")) return false;
            if (!generate_expression(gen, expr->data.binary.right)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_NOT:
            if (!codegen_append(gen, "!(")) return false;
            if (!generate_expression(gen, expr->data.unary.operand)) return false;
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        case EXPR_CALL:
            /* Special case for print */
            if (strcmp(expr->data.call.func_name, "print") == 0) {
                if (!codegen_append(gen, "printf(\"%d\\n\", ")) return false;
                if (expr->data.call.arg_count > 0) {
                    if (!generate_expression(gen, expr->data.call.args[0])) return false;
                }
                if (!codegen_append(gen, ")")) return false;
                return true;
            }
            
            /* Regular function call */
            if (!codegen_append(gen, expr->data.call.func_name)) return false;
            if (!codegen_append(gen, "(")) return false;
            
            for (size_t i = 0; i < expr->data.call.arg_count; i++) {
                if (i > 0) {
                    if (!codegen_append(gen, ", ")) return false;
                }
                if (!generate_expression(gen, expr->data.call.args[i])) return false;
            }
            
            if (!codegen_append(gen, ")")) return false;
            return true;
            
        default:
            return false;
    }
}

/* ==============================================================================
 * C Code Generation - Statements
 * ==============================================================================
 */

static bool generate_statement(CodeGen *gen, ASTStmt *stmt) {
    if (!stmt) return false;
    
    switch (stmt->kind) {
        case STMT_VAR_DECL:
            if (!codegen_indent(gen)) return false;
            if (!codegen_append(gen, type_to_string(stmt->data.var_decl.type))) return false;
            if (!codegen_append(gen, " ")) return false;
            if (!codegen_append(gen, stmt->data.var_decl.name)) return false;
            if (!codegen_append(gen, " = ")) return false;
            if (!generate_expression(gen, stmt->data.var_decl.init_expr)) return false;
            if (!codegen_append(gen, ";\n")) return false;
            return true;
            
        case STMT_ASSIGN:
            if (!codegen_indent(gen)) return false;
            if (!codegen_append(gen, stmt->data.assign.name)) return false;
            if (!codegen_append(gen, " = ")) return false;
            if (!generate_expression(gen, stmt->data.assign.expr)) return false;
            if (!codegen_append(gen, ";\n")) return false;
            return true;
            
        case STMT_IF:
            if (!codegen_indent(gen)) return false;
            if (!codegen_append(gen, "if (")) return false;
            if (!generate_expression(gen, stmt->data.if_stmt.condition)) return false;
            if (!codegen_append(gen, ") ")) return false;
            
            if (!generate_statement(gen, stmt->data.if_stmt.then_block)) return false;
            
            if (stmt->data.if_stmt.else_block) {
                if (!codegen_indent(gen)) return false;
                if (!codegen_append(gen, "else ")) return false;
                if (!generate_statement(gen, stmt->data.if_stmt.else_block)) return false;
            }
            
            return true;
            
        case STMT_WHILE:
            if (!codegen_indent(gen)) return false;
            if (!codegen_append(gen, "while (")) return false;
            if (!generate_expression(gen, stmt->data.while_stmt.condition)) return false;
            if (!codegen_append(gen, ") ")) return false;
            if (!generate_statement(gen, stmt->data.while_stmt.body)) return false;
            return true;
            
        case STMT_RETURN:
            if (!codegen_indent(gen)) return false;
            if (!codegen_append(gen, "return")) return false;
            
            if (stmt->data.return_stmt.expr) {
                if (!codegen_append(gen, " ")) return false;
                if (!generate_expression(gen, stmt->data.return_stmt.expr)) return false;
            }
            
            if (!codegen_append(gen, ";\n")) return false;
            return true;
            
        case STMT_EXPR:
            if (!codegen_indent(gen)) return false;
            if (!generate_expression(gen, stmt->data.expr_stmt.expr)) return false;
            if (!codegen_append(gen, ";\n")) return false;
            return true;
            
        case STMT_BLOCK:
            if (!codegen_append(gen, "{\n")) return false;
            gen->indent_level++;
            
            for (size_t i = 0; i < stmt->data.block.stmt_count; i++) {
                if (!generate_statement(gen, stmt->data.block.statements[i])) {
                    gen->indent_level--;
                    return false;
                }
            }
            
            gen->indent_level--;
            if (!codegen_indent(gen)) return false;
            if (!codegen_append(gen, "}\n")) return false;
            return true;
            
        default:
            return false;
    }
}

/* ==============================================================================
 * C Code Generation - Functions
 * ==============================================================================
 */

static bool generate_function(CodeGen *gen, ASTFunc *func) {
    /* Return type */
    if (!codegen_append(gen, type_to_string(func->return_type))) return false;
    if (!codegen_append(gen, " ")) return false;
    
    /* Function name */
    if (!codegen_append(gen, func->name)) return false;
    if (!codegen_append(gen, "(")) return false;
    
    /* Parameters */
    if (func->param_count == 0) {
        if (!codegen_append(gen, "void")) return false;
    } else {
        for (size_t i = 0; i < func->param_count; i++) {
            if (i > 0) {
                if (!codegen_append(gen, ", ")) return false;
            }
            
            if (!codegen_append(gen, type_to_string(func->params[i].type))) return false;
            if (!codegen_append(gen, " ")) return false;
            if (!codegen_append(gen, func->params[i].name)) return false;
        }
    }
    
    if (!codegen_append(gen, ") ")) return false;
    
    /* Body */
    if (!generate_statement(gen, func->body)) return false;
    
    if (!codegen_append(gen, "\n")) return false;
    
    return true;
}

/* ==============================================================================
 * C Code Generation - Program
 * ==============================================================================
 */

static bool generate_program_c(CodeGen *gen, ASTProgram *program) {
    /* Header */
    if (gen->config && gen->config->emit_comments) {
        if (!codegen_append(gen, "/* Generated by TinyLLVM Compiler */\n\n")) return false;
    }
    
    if (!codegen_append(gen, "#include <stdio.h>\n")) return false;
    if (!codegen_append(gen, "#include <stdbool.h>\n\n")) return false;
    
    /* Forward declarations */
    for (size_t i = 0; i < program->func_count; i++) {
        ASTFunc *func = program->functions[i];
        
        if (!codegen_append(gen, type_to_string(func->return_type))) return false;
        if (!codegen_append(gen, " ")) return false;
        if (!codegen_append(gen, func->name)) return false;
        if (!codegen_append(gen, "(")) return false;
        
        if (func->param_count == 0) {
            if (!codegen_append(gen, "void")) return false;
        } else {
            for (size_t j = 0; j < func->param_count; j++) {
                if (j > 0) {
                    if (!codegen_append(gen, ", ")) return false;
                }
                if (!codegen_append(gen, type_to_string(func->params[j].type))) return false;
            }
        }
        
        if (!codegen_append(gen, ");\n")) return false;
    }
    
    if (!codegen_append(gen, "\n")) return false;
    
    /* Function definitions */
    for (size_t i = 0; i < program->func_count; i++) {
        if (!generate_function(gen, program->functions[i])) return false;
    }
    
    return true;
}

/* ==============================================================================
 * Public Code Generator API
 * ==============================================================================
 */

char *generate_c_code(ASTProgram *program, CompilerConfig *config) {
    if (!program) return NULL;
    
    CodeGen gen = {
        .output = NULL,
        .length = 0,
        .capacity = 0,
        .indent_level = 0,
        .config = config
    };
    
    if (!generate_program_c(&gen, program)) {
        free(gen.output);
        return NULL;
    }
    
    return gen.output;
}

/* ==============================================================================
 * Code Generator Event (EventChains Integration)
 * ==============================================================================
 */

EventResult compiler_codegen_event(EventContext *context, void *user_data) {
    CompilerConfig *config = (CompilerConfig *)user_data;
    
    EventResult result;
    
    /* Get AST from context */
    ASTProgram *program;
    EventChainErrorCode err = event_context_get(context, "ast", (void**)&program);
    
    if (err != EC_SUCCESS || !program) {
        event_result_failure(&result, "No AST provided to code generator",
                           EC_ERROR_INVALID_PARAMETER, ERROR_DETAIL_FULL);
        return result;
    }
    
    /* Generate code based on target */
    char *output = NULL;
    
    if (!config || config->target == TARGET_C || config->target == TARGET_TINYLLVM) {
        output = generate_c_code(program, config);
    } else {
        event_result_failure(&result, "Unsupported code generation target",
                           EC_ERROR_INVALID_PARAMETER, ERROR_DETAIL_FULL);
        return result;
    }
    
    if (!output) {
        event_result_failure(&result, "Code generation failed",
                           EC_ERROR_OUT_OF_MEMORY, ERROR_DETAIL_FULL);
        return result;
    }
    
    /* Store output in context */
    err = event_context_set_with_cleanup(context, "output_code", output, free);
    
    if (err != EC_SUCCESS) {
        free(output);
        event_result_failure(&result, "Failed to store output code in context",
                           err, ERROR_DETAIL_FULL);
        return result;
    }
    
    /* Success */
    event_result_success(&result);
    return result;
}
