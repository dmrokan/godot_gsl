#include "godot_gsl_function.h"

#define ARGV_BOUND_DATA_COUNT 4
#define GGSL_ARGV_BOUNDS_ALLOC(__var, size)                           \
    {                                                                 \
    size_t byte_size = size * sizeof(size_t) * ARGV_BOUND_DATA_COUNT; \
    uint8_t *mem_tmp = memnew_arr(uint8_t, byte_size);                \
    __var = (size_t**) mem_tmp;                                       \
    }                                                                 \


#define GGSL_ARGV_BOUNDS_SET(__var, __bounds, k)                        \
    {                                                                   \
        memcpy(&__var[k], __bounds, sizeof(GGSL_BOUNDS));               \
    }                                                                   \


typedef enum GGSL_PARSER_STATE {
    S_IDLE,
    S_NUMBER,
    S_SEP,
} GGSL_PARSER_STATE;

GGSL_BOUNDS argv_bounds_parse(const String vn, size_t *size)
{
    GGSL_PARSER_STATE state = S_IDLE;
    size_t bounds[ARGV_BOUND_DATA_COUNT];
    bounds[0] = bounds[2] = 0;
    bounds[1] = size[0];
    bounds[3] = size[1];

    int l = 0;
    String sub = "";
    for (int k = 0; k < vn.size(); k++)
    {
        char c = (vn[k]);

        if (state == S_IDLE)
        {
            if (c == '[')
            {
                state = S_NUMBER;
                continue;
            }
        }
        else if (state == S_NUMBER)
        {
            if (c >= '0' && c <= '9')
            {
                sub += c;
                continue;
            }
            else if (c == ':' || c == ',')
            {
                bounds[l] = sub.to_int();
                l++;
                state = S_SEP;
                continue;
            }
            else if (c == ']')
            {
                bounds[l] = sub.to_int();
                state = S_IDLE;
                break;
            }
        }
        else if (state == S_SEP)
        {
            sub.clear();
            k--;
            state = S_NUMBER;
            continue;
        }
    }

    GGSL_BOUNDS _bounds;
    _bounds.r1 = bounds[0];
    _bounds.r2 = bounds[1];
    _bounds.c1 = bounds[2];
    _bounds.c2 = bounds[3];
    return _bounds;
}

String remove_whitespace(const String str)
{
    String result = str;
    result.replace(" ", "");
    result.replace("\t", "");
    result.replace("\n", "");
    result.replace("\r", "");

    return result;
}


/***** GodotGSLFunction methods *****/

GodotGSLFunction::GodotGSLFunction(const String fn)
{
    name = fn;
    GGSL_ALLOC(argv, INITIAL_ARGV_COUNT);
    GGSL_ALLOC_G(instructions, INITIAL_INS_COUNT, GodotGSLInstruction);
}

GodotGSLFunction::~GodotGSLFunction()
{
    for (int k = 0; k < instruction_count; k++)
    {
        memdelete(instructions[k]);
    }

    for (int k = 0; k < argv_buffer_size; k++)
    {
        if (argv[k] != NULL)
        {
            memdelete(argv[k]);
        }
    }

    for (int k = 0; k < instruction_buffer_size; k++)
    {
        if (instructions[k] != NULL)
        {
            memdelete(instructions[k]);
        }
    }
}

void GodotGSLFunction::add_arguments(const Array args, GodotGSLMatrix **a)
{
    arg_names = args;

    /*
     * TODO: There is no need to backup here
     * No new insturctions are added
     * All added at once
     */

    for (int k = 0; k < args.size(); k++)
    {
        add_argument(arg_names[k], a[k]);
    }
}

void GodotGSLFunction::add_argument(const String vn, GodotGSLMatrix *a)
{
    if (argc + 1 > argv_buffer_size)
    {
        GodotGSLMatrix **new_argv;
        GGSL_ALLOC(new_argv, (argv_buffer_size + INITIAL_ARGV_COUNT));
        memcpy(new_argv, argv, argv_buffer_size * sizeof(GodotGSLMatrix*));
        GodotGSLMatrix **to_rm = argv;
        argv = new_argv;
        GGSL_FREE(to_rm);
        argv_buffer_size += INITIAL_ARGV_COUNT;
    }

    argv[argc] = a;
    arg_names.append(vn);

    argc++;
}

void GodotGSLFunction::add_instruction(const String in, const Array args)
{
    if (instruction_count + 1 > instruction_buffer_size)
    {
        GodotGSLInstruction **new_instructions;
        GGSL_ALLOC_G(new_instructions, (instruction_buffer_size + INITIAL_INS_COUNT), GodotGSLInstruction);
        memcpy(new_instructions, instructions, instruction_buffer_size * sizeof(GodotGSLInstruction*));
        GodotGSLInstruction **to_rm = instructions;
        instructions = new_instructions;
        GGSL_FREE(to_rm);
        instruction_buffer_size += INITIAL_INS_COUNT;
    }

    GodotGSLInstruction *ins = memnew(GodotGSLInstruction(in));
    set_instruction_arguments(ins, args);
    instructions[instruction_count] = ins;
    instruction_count++;

    if (current == NULL)
    {
        current = ins;
    }
    else
    {
        current->nxt = ins;
        current = ins;
    }
}

void GodotGSLFunction::set_instruction_arguments(Array args)
{
    if (current == NULL)
    {
        GGSL_MESSAGE("GodotGSLFunction::set_instruction_arguments: current == NULL");
        return;
    }

    set_instruction_arguments(current, args);
}

void GodotGSLFunction::set_instruction_arguments(GodotGSLInstruction *ins, Array args)
{
    if (ins == NULL)
    {
        GGSL_MESSAGE("GodotGSLFunction::set_instruction_arguments: ins == NULL");
        return;
    }

    int size = args.size();
    if (size == 0 || size > argc)
    {
        GGSL_MESSAGE("GodotGSLFunction::set_instruction_arguments: size == 0 || size > argc");
        return;
    }

    for (int k = 0; k < size; k++)
    {
        String arg = remove_whitespace(args[k]);
        int sindex = arg.find_char('[');
        if (sindex < 0)
        {
            sindex = arg.size();
        }

        String vn = arg.substr(0, sindex);
        int index = arg_names.find(vn);

        if (index > -1)
        {
            ins->argv[k] = argv[index];
            GodotGSLMatrix *argv_mtx = argv[index];
            GGSL_BOUNDS bounds = argv_bounds_parse(arg, argv_mtx->size);
            if (!argv_mtx->is_bounds_included(bounds))
            {
                GGSL_ERR_MESSAGE("GodotGSLFunction::set_instruction_arguments: !argv_mtx->is_bounds_included(bounds)");
                return;
            }
            /* TODO: memcpy may no be the right wway to do this */
            GGSL_ARGV_BOUNDS_SET(ins->argv_bounds, &bounds, k);
        }
        else
        {
            GGSL_MESSAGE("GodotGSLFunction::set_instruction_arguments: !arg_names.has(vn)");
            return;
        }
    }

    ins->argc = size;
}

void GodotGSLFunction::execute()
{
    if (instructions == NULL)
    {
        GGSL_MESSAGE("GodotGSLFunction::execute: instructions == NULL");
        return;
    }

    GodotGSLInstruction *first = instructions[0];

    do {
        first->execute();
    } while ((first = first->next()));
}

GodotGSLMatrix *GodotGSLFunction::get_arg(const String vn)
{
    int index = arg_names.find(vn);

    if (index < 0)
    {
        GGSL_ERR_MESSAGE("GodotGSLFunction::get_arg: index < 0");
        return NULL;
    }

    return argv[index];
}

GodotGSLMatrix *GodotGSLFunction::get_arg(const size_t index)
{
    if (index >= argc)
    {
        GGSL_ERR_MESSAGE("GodotGSLFunction::get_arg: index >= argc");
        return NULL;
    }

    return argv[index];
}
