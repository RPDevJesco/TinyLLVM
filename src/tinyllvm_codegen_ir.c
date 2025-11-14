/**
 * ==============================================================================
 * TinyLLVM Compiler - IR Code Generator
 * ==============================================================================
 *
 * Generates TinyLLVM IR (human-readable intermediate representation) from AST.
 *
 * IR Format:
 *   - SSA-like format with explicit temporaries
 *   - Simple instruction set: load, store, add, sub, mul, div, etc.
 *   - Labels for control flow
 *   - Function definitions with entry/exit blocks
 */

#include "include/tinyllvm_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ==============================================================================
 * IR Generation State
 * ==============================================================================
 */

typedef struct {
    char *output;
    size_t length;
    size_t capacity;
    int indent_level;

    /* Temporary variable counter */
    int temp_counter;

    /* Label counter */
    int label_counter;

    CompilerConfig *config;
} IRCodeGen;

static bool ir_codegen_grow(IRCodeGen *gen, size_t additional) {
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

static bool ir_codegen_append(IRCodeGen *gen, const char *str) {
    size_t len = strlen(str);
    if (!ir_codegen_grow(gen, len)) return false;

    memcpy(gen->output + gen->length, str, len);
    gen->length += len;
    gen->output[gen->length] = '\0';

    return true;
}

static bool ir_codegen_appendf(IRCodeGen *gen, const char *format, ...) {
    char buffer[1024];

    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len < 0 || (size_t)len >= sizeof(buffer)) {
        return false;
    }

    return ir_codegen_append(gen, buffer);
}

static bool ir_codegen_indent(IRCodeGen *gen) {
    for (int i = 0; i < gen->indent_level; i++) {
        if (!ir_codegen_append(gen, "  ")) return false;
    }
    return true;
}

static int ir_get_next_temp(IRCodeGen *gen) {
    return gen->temp_counter++;
}

static int ir_get_next_label(IRCodeGen *gen) {
    return gen->label_counter++;
}

/* ==============================================================================
 * Forward Declarations
 * ==============================================================================
 */

static int ir_generate_expression(IRCodeGen *gen, ASTExpr *expr);
static bool ir_generate_statement(IRCodeGen *gen, ASTStmt *stmt);

/* ==============================================================================
 * IR Code Generation - Expressions
 * ==============================================================================
 */

static int ir_generate_expression(IRCodeGen *gen, ASTExpr *expr) {
    if (!expr) return -1;

    int result_temp = ir_get_next_temp(gen);

    switch (expr->kind) {
        case EXPR_INT_LITERAL:
            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = const i32 %d\n",
                                   result_temp, expr->data.int_lit.value)) return -1;
            return result_temp;

        case EXPR_BOOL_LITERAL:
            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = const i1 %d\n",
                                   result_temp, expr->data.bool_lit.value ? 1 : 0)) return -1;
            return result_temp;

        case EXPR_VAR:
            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = load %%%s\n",
                                   result_temp, expr->data.var.name)) return -1;
            return result_temp;

        case EXPR_ADD: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = add i32 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_SUB: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = sub i32 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_MUL: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = mul i32 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_DIV: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = div i32 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_MOD: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = mod i32 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_EQ: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = icmp eq i32 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_NE: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = icmp ne i32 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_LT: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = icmp lt i32 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_LE: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = icmp le i32 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_GT: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = icmp gt i32 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_GE: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = icmp ge i32 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_AND: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = and i1 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_OR: {
            int left_temp = ir_generate_expression(gen, expr->data.binary.left);
            if (left_temp < 0) return -1;
            int right_temp = ir_generate_expression(gen, expr->data.binary.right);
            if (right_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = or i1 %%t%d, %%t%d\n",
                                   result_temp, left_temp, right_temp)) return -1;
            return result_temp;
        }

        case EXPR_NOT: {
            int operand_temp = ir_generate_expression(gen, expr->data.unary.operand);
            if (operand_temp < 0) return -1;

            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = xor i1 %%t%d, 1\n",
                                   result_temp, operand_temp)) return -1;
            return result_temp;
        }

        case EXPR_CALL: {
            /* Special case for print */
            if (strcmp(expr->data.call.func_name, "print") == 0) {
                if (expr->data.call.arg_count > 0) {
                    int arg_temp = ir_generate_expression(gen, expr->data.call.args[0]);
                    if (arg_temp < 0) return -1;

                    if (!ir_codegen_indent(gen)) return -1;
                    if (!ir_codegen_appendf(gen, "call void @print(i32 %%t%d)\n", arg_temp)) return -1;
                }
                return result_temp;
            }

            /* Regular function call - first generate all arguments */
            int arg_temps[16];
            for (size_t i = 0; i < expr->data.call.arg_count && i < 16; i++) {
                arg_temps[i] = ir_generate_expression(gen, expr->data.call.args[i]);
                if (arg_temps[i] < 0) return -1;
            }

            /* Generate call instruction */
            if (!ir_codegen_indent(gen)) return -1;
            if (!ir_codegen_appendf(gen, "%%t%d = call i32 @%s(",
                                   result_temp, expr->data.call.func_name)) return -1;

            for (size_t i = 0; i < expr->data.call.arg_count && i < 16; i++) {
                if (i > 0) {
                    if (!ir_codegen_append(gen, ", ")) return -1;
                }
                if (!ir_codegen_appendf(gen, "i32 %%t%d", arg_temps[i])) return -1;
            }

            if (!ir_codegen_append(gen, ")\n")) return -1;
            return result_temp;
        }

        default:
            return -1;
    }
}

/* ==============================================================================
 * IR Code Generation - Statements
 * ==============================================================================
 */

static bool ir_generate_statement(IRCodeGen *gen, ASTStmt *stmt) {
    if (!stmt) return false;

    switch (stmt->kind) {
        case STMT_VAR_DECL: {
            /* Allocate space for variable */
            if (!ir_codegen_indent(gen)) return false;
            if (!ir_codegen_appendf(gen, "%%%s = alloca i32\n", stmt->data.var_decl.name)) return false;

            /* Generate initialization expression */
            int init_temp = ir_generate_expression(gen, stmt->data.var_decl.init_expr);
            if (init_temp < 0) return false;

            /* Store initial value */
            if (!ir_codegen_indent(gen)) return false;
            if (!ir_codegen_appendf(gen, "store i32 %%t%d, %%%s\n",
                                   init_temp, stmt->data.var_decl.name)) return false;
            return true;
        }

        case STMT_ASSIGN: {
            /* Generate expression */
            int expr_temp = ir_generate_expression(gen, stmt->data.assign.expr);
            if (expr_temp < 0) return false;

            /* Store to variable */
            if (!ir_codegen_indent(gen)) return false;
            if (!ir_codegen_appendf(gen, "store i32 %%t%d, %%%s\n",
                                   expr_temp, stmt->data.assign.name)) return false;
            return true;
        }

        case STMT_IF: {
            /* Generate condition */
            int cond_temp = ir_generate_expression(gen, stmt->data.if_stmt.condition);
            if (cond_temp < 0) return false;

            /* Create labels */
            int then_label = ir_get_next_label(gen);
            int else_label = ir_get_next_label(gen);
            int end_label = ir_get_next_label(gen);

            /* Branch instruction */
            if (!ir_codegen_indent(gen)) return false;
            if (stmt->data.if_stmt.else_block) {
                if (!ir_codegen_appendf(gen, "br i1 %%t%d, label %%L%d, label %%L%d\n",
                                       cond_temp, then_label, else_label)) return false;
            } else {
                if (!ir_codegen_appendf(gen, "br i1 %%t%d, label %%L%d, label %%L%d\n",
                                       cond_temp, then_label, end_label)) return false;
            }

            /* Then block */
            if (!ir_codegen_append(gen, "\n")) return false;
            if (!ir_codegen_appendf(gen, "L%d:\n", then_label)) return false;
            gen->indent_level++;
            if (!ir_generate_statement(gen, stmt->data.if_stmt.then_block)) {
                gen->indent_level--;
                return false;
            }
            gen->indent_level--;
            if (!ir_codegen_indent(gen)) return false;
            if (!ir_codegen_appendf(gen, "br label %%L%d\n", end_label)) return false;

            /* Else block (if present) */
            if (stmt->data.if_stmt.else_block) {
                if (!ir_codegen_append(gen, "\n")) return false;
                if (!ir_codegen_appendf(gen, "L%d:\n", else_label)) return false;
                gen->indent_level++;
                if (!ir_generate_statement(gen, stmt->data.if_stmt.else_block)) {
                    gen->indent_level--;
                    return false;
                }
                gen->indent_level--;
                if (!ir_codegen_indent(gen)) return false;
                if (!ir_codegen_appendf(gen, "br label %%L%d\n", end_label)) return false;
            }

            /* End label */
            if (!ir_codegen_append(gen, "\n")) return false;
            if (!ir_codegen_appendf(gen, "L%d:\n", end_label)) return false;
            return true;
        }

        case STMT_WHILE: {
            /* Create labels */
            int cond_label = ir_get_next_label(gen);
            int body_label = ir_get_next_label(gen);
            int end_label = ir_get_next_label(gen);

            /* Jump to condition */
            if (!ir_codegen_indent(gen)) return false;
            if (!ir_codegen_appendf(gen, "br label %%L%d\n", cond_label)) return false;

            /* Condition block */
            if (!ir_codegen_append(gen, "\n")) return false;
            if (!ir_codegen_appendf(gen, "L%d:\n", cond_label)) return false;
            gen->indent_level++;
            int cond_temp = ir_generate_expression(gen, stmt->data.while_stmt.condition);
            if (cond_temp < 0) {
                gen->indent_level--;
                return false;
            }
            if (!ir_codegen_indent(gen)) return false;
            if (!ir_codegen_appendf(gen, "br i1 %%t%d, label %%L%d, label %%L%d\n",
                                   cond_temp, body_label, end_label)) return false;
            gen->indent_level--;

            /* Body block */
            if (!ir_codegen_append(gen, "\n")) return false;
            if (!ir_codegen_appendf(gen, "L%d:\n", body_label)) return false;
            gen->indent_level++;
            if (!ir_generate_statement(gen, stmt->data.while_stmt.body)) {
                gen->indent_level--;
                return false;
            }
            if (!ir_codegen_indent(gen)) return false;
            if (!ir_codegen_appendf(gen, "br label %%L%d\n", cond_label)) return false;
            gen->indent_level--;

            /* End label */
            if (!ir_codegen_append(gen, "\n")) return false;
            if (!ir_codegen_appendf(gen, "L%d:\n", end_label)) return false;
            return true;
        }

        case STMT_RETURN: {
            if (stmt->data.return_stmt.expr) {
                int expr_temp = ir_generate_expression(gen, stmt->data.return_stmt.expr);
                if (expr_temp < 0) return false;

                if (!ir_codegen_indent(gen)) return false;
                if (!ir_codegen_appendf(gen, "ret i32 %%t%d\n", expr_temp)) return false;
            } else {
                if (!ir_codegen_indent(gen)) return false;
                if (!ir_codegen_append(gen, "ret void\n")) return false;
            }
            return true;
        }

        case STMT_EXPR: {
            int expr_temp = ir_generate_expression(gen, stmt->data.expr_stmt.expr);
            return expr_temp >= 0;
        }

        case STMT_BLOCK:
            for (size_t i = 0; i < stmt->data.block.stmt_count; i++) {
                if (!ir_generate_statement(gen, stmt->data.block.statements[i])) {
                    return false;
                }
            }
            return true;

        default:
            return false;
    }
}

/* ==============================================================================
 * IR Code Generation - Functions
 * ==============================================================================
 */

static bool ir_generate_function(IRCodeGen *gen, ASTFunc *func) {
    /* Function signature */
    const char *return_type = (func->return_type.kind == TYPE_INT) ? "i32" :
                              (func->return_type.kind == TYPE_BOOL) ? "i1" : "void";

    if (!ir_codegen_appendf(gen, "define %s @%s(", return_type, func->name)) return false;

    /* Parameters */
    for (size_t i = 0; i < func->param_count; i++) {
        if (i > 0) {
            if (!ir_codegen_append(gen, ", ")) return false;
        }

        const char *param_type = (func->params[i].type.kind == TYPE_INT) ? "i32" :
                                 (func->params[i].type.kind == TYPE_BOOL) ? "i1" : "i32";

        if (!ir_codegen_appendf(gen, "%s %%%s.param", param_type, func->params[i].name)) return false;
    }

    if (!ir_codegen_append(gen, ") {\n")) return false;

    /* Entry label */
    if (!ir_codegen_append(gen, "entry:\n")) return false;

    gen->indent_level++;

    /* Allocate space for parameters and copy values */
    for (size_t i = 0; i < func->param_count; i++) {
        if (!ir_codegen_indent(gen)) {
            gen->indent_level--;
            return false;
        }
        if (!ir_codegen_appendf(gen, "%%%s = alloca i32\n", func->params[i].name)) {
            gen->indent_level--;
            return false;
        }
        if (!ir_codegen_indent(gen)) {
            gen->indent_level--;
            return false;
        }
        if (!ir_codegen_appendf(gen, "store i32 %%%s.param, %%%s\n",
                               func->params[i].name, func->params[i].name)) {
            gen->indent_level--;
            return false;
        }
    }

    /* Function body */
    if (!ir_generate_statement(gen, func->body)) {
        gen->indent_level--;
        return false;
    }

    gen->indent_level--;

    if (!ir_codegen_append(gen, "}\n\n")) return false;

    return true;
}

/* ==============================================================================
 * IR Code Generation - Program
 * ==============================================================================
 */

static bool ir_generate_program(IRCodeGen *gen, ASTProgram *program) {
    /* Header */
    if (gen->config && gen->config->emit_comments) {
        if (!ir_codegen_append(gen, "; Generated by TinyLLVM Compiler\n")) return false;
        if (!ir_codegen_append(gen, "; Target: TinyLLVM IR (human-readable)\n\n")) return false;
    }

    /* Declare print function */
    if (!ir_codegen_append(gen, "declare void @print(i32)\n\n")) return false;

    /* Generate all functions */
    for (size_t i = 0; i < program->func_count; i++) {
        if (!ir_generate_function(gen, program->functions[i])) return false;
    }

    return true;
}

/* ==============================================================================
 * Public IR Code Generator API
 * ==============================================================================
 */

char *generate_ir_code(ASTProgram *program, CompilerConfig *config) {
    if (!program) return NULL;

    IRCodeGen gen = {
        .output = NULL,
        .length = 0,
        .capacity = 0,
        .indent_level = 0,
        .temp_counter = 0,
        .label_counter = 0,
        .config = config
    };

    if (!ir_generate_program(&gen, program)) {
        free(gen.output);
        return NULL;
    }

    return gen.output;
}