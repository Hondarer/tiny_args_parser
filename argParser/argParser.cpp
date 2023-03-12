// 制約: 同じオプションのkey-valueの出現は1回のみ

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define PARAM_REQUIRED (0x00000001)

#define PARAM_STRING (0x00000010)
#define PARAM_INT (0x00000020)

#define PARAM_INDEXED (0x00000100)
#define PARAM_FLAG (0x00000200)
#define PARAM_KEYVALUE (0x00000400)

#define PARAM_CONFIG_MAX (10)

static int _argc = 0;
static char** _argv = NULL;

struct param_config
{
    unsigned int flags = 0;
    int index = 0;
    int found = 0;
    const char* short_option = NULL;
    const char* long_option = NULL;
    char** string_storage = NULL;
    int* int_storage = NULL;
};

static param_config param_configs[PARAM_CONFIG_MAX];

static int param_configs_initialized = 0;
static int _param_indexed_index = 1;
static int _param_not_indexed_require_detected = 0;

static int is_number(char* check_string)
{
    for (int index = 0; index < strlen(check_string); index++)
    {
        if (check_string[index] < '0' || '9' < check_string[index])
        {
            return -1;
        }
    }
    return 0;
}

static int parse_param(const int argc, char** argv)
{
    _argc = argc;
    _argv = argv;

    int indexed_param_index = 1;
    int processing_config_index = -1;

    for (int arg_index = 1; arg_index < argc; arg_index++)
    {
        int unknown_arg = 1;

        if (processing_config_index != -1)
        {
            if ((param_configs[processing_config_index].flags & PARAM_STRING) == PARAM_STRING)
            {
                *param_configs[processing_config_index].string_storage = argv[arg_index];
                param_configs[processing_config_index].found = 1;
                unknown_arg = 0;
            }
            if ((param_configs[processing_config_index].flags & PARAM_INT) == PARAM_INT)
            {
                if (is_number(argv[arg_index]) == 0)
                {
                    *param_configs[processing_config_index].int_storage = atoi(argv[arg_index]);
                    param_configs[processing_config_index].found = 1;
                    unknown_arg = 0;
                }
                else
                {
                    printf("The value of option %s(%s) must be an int.\n", param_configs[processing_config_index].long_option, param_configs[processing_config_index].short_option);
                    return -1;
                }
            }
            processing_config_index = -1;
            continue;
        }

        for (int param_config_index = 0; param_config_index < PARAM_CONFIG_MAX; param_config_index++)
        {
            if ((param_configs[param_config_index].flags & PARAM_FLAG) == PARAM_FLAG)
            {
                if (param_configs[param_config_index].short_option != NULL && strcmp(argv[arg_index], param_configs[param_config_index].short_option) == 0)
                {
                    *param_configs[param_config_index].int_storage = 1;
                    param_configs[param_config_index].found = 1;
                    unknown_arg = 0;
                    break;
                }
                if (param_configs[param_config_index].long_option != NULL && strcmp(argv[arg_index], param_configs[param_config_index].long_option) == 0)
                {
                    *param_configs[param_config_index].int_storage = 1;
                    param_configs[param_config_index].found = 1;
                    unknown_arg = 0;
                    break;
                }
            }

            if ((param_configs[param_config_index].flags & PARAM_KEYVALUE) == PARAM_KEYVALUE)
            {
                if (param_configs[param_config_index].short_option != NULL && strcmp(argv[arg_index], param_configs[param_config_index].short_option) == 0)
                {
                    processing_config_index = param_config_index;
                    unknown_arg = 0;
                    break;
                }
                if (param_configs[param_config_index].long_option != NULL && strcmp(argv[arg_index], param_configs[param_config_index].long_option) == 0)
                {
                    processing_config_index = param_config_index;
                    unknown_arg = 0;
                    break;
                }
            }
        }

        if (unknown_arg == 1)
        {
            for (int param_config_index = 0; param_config_index < PARAM_CONFIG_MAX; param_config_index++)
            {
                if ((param_configs[param_config_index].flags & PARAM_INDEXED) == PARAM_INDEXED &&
                    param_configs[param_config_index].index == indexed_param_index)
                {
                    if ((param_configs[param_config_index].flags & PARAM_STRING) == PARAM_STRING)
                    {
                        *param_configs[param_config_index].string_storage = argv[arg_index];
                        param_configs[param_config_index].found = 1;
                        unknown_arg = 0;
                        indexed_param_index++;
                        break;
                    }
                    if ((param_configs[param_config_index].flags & PARAM_INT) == PARAM_INT)
                    {
                        if (is_number(argv[arg_index]) == 0)
                        {
                            *param_configs[param_config_index].int_storage = atoi(argv[arg_index]);
                            param_configs[param_config_index].found = 1;
                            unknown_arg = 0;
                            indexed_param_index++;
                            break;
                        }
                        else
                        {
                            printf("Argument %d must be an int.\n", indexed_param_index);
                            return -1;
                        }
                    }
                }
            }
        }

        if (unknown_arg == 1)
        {
            printf("Unknown argument %s.\n", argv[arg_index]);
            return -1;
        }
    }

    for (int param_config_index = 0; param_config_index < PARAM_CONFIG_MAX; param_config_index++)
    {
        if (param_configs[param_config_index].flags == 0)
        {
            break;
        }
        if ((param_configs[param_config_index].flags & PARAM_REQUIRED) != PARAM_REQUIRED)
        {
            continue;
        }
        if (param_configs[param_config_index].found == 0)
        {
            if ((param_configs[param_config_index].flags & PARAM_INDEXED) == PARAM_INDEXED)
            {
                printf("Argument %d is required.\n", param_configs[param_config_index].index);
            }
            if ((param_configs[param_config_index].flags & PARAM_KEYVALUE) == PARAM_KEYVALUE)
            {
                printf("Optional value %s(%s) is required.\n", param_configs[param_config_index].long_option, param_configs[param_config_index].short_option);
            }
            return -1;
        }
    }
    return 0;
}

static void regist_param_core(const unsigned int flags, char** string_storage, int* int_storage, const  char* short_option, const  char* long_option)
{
    if (param_configs_initialized == 0)
    {
        memset(&param_configs[0], 0x00, sizeof(param_configs));
        param_configs_initialized = 1;
    }

    for (int i = 0; i < PARAM_CONFIG_MAX; i++)
    {
        if (param_configs[i].flags != 0)
        {
            continue;
        }
        if (i == (PARAM_CONFIG_MAX - 1))
        {
            printf("The maximum number of parsable arguments (%d) has been exceeded.\n", PARAM_CONFIG_MAX);
        }

        if ((flags & PARAM_STRING) == PARAM_STRING)
        {
            if (string_storage == NULL)
            {
                printf("string_storage is NULL.\n");
                return;
            }
            param_configs[i].string_storage = string_storage;
        }
        if ((flags & PARAM_INT) == PARAM_INT || (flags & PARAM_FLAG) == PARAM_FLAG)
        {
            if (int_storage == NULL)
            {
                printf("int_storage is NULL.\n");
                return;
            }
            param_configs[i].int_storage = int_storage;
        }
        if ((flags & PARAM_REQUIRED) == PARAM_REQUIRED && (flags & PARAM_INDEXED) == PARAM_INDEXED && _param_not_indexed_require_detected == 1)
        {
            printf("Required argument set after a non-mandatory argument.\n");
            return;
        }

        param_configs[i].flags = flags;
        param_configs[i].short_option = short_option;
        param_configs[i].long_option = long_option;
        if ((flags & PARAM_INDEXED) == PARAM_INDEXED)
        {
            param_configs[i].index = _param_indexed_index;
            _param_indexed_index++;
        }
        if ((flags & PARAM_REQUIRED) != PARAM_REQUIRED && (flags & PARAM_INDEXED) == PARAM_INDEXED)
        {
            _param_not_indexed_require_detected = 1;
        }
        break;
    }
}

static void regist_param_indexed_string_required(char** storage)
{
    regist_param_core(PARAM_INDEXED | PARAM_STRING | PARAM_REQUIRED, storage, NULL, NULL, NULL);
}

static void regist_param_indexed_int_required(int* storage)
{
    regist_param_core(PARAM_INDEXED | PARAM_INT | PARAM_REQUIRED, NULL, storage, NULL, NULL);
}

static void regist_param_indexed_string(char** storage)
{
    regist_param_core(PARAM_INDEXED | PARAM_STRING, storage, NULL, NULL, NULL);
}

static void regist_param_indexed_int(int* storage)
{
    regist_param_core(PARAM_INDEXED | PARAM_INT, NULL, storage, NULL, NULL);
}

static void regist_param_flag(const char* short_option, const char* long_option, int* storage)
{
    regist_param_core(PARAM_FLAG, NULL, storage, short_option, long_option);
}

static void regist_param_keyvalue_string_required(const char* short_option, const char* long_option, char** storage)
{
    regist_param_core(PARAM_KEYVALUE | PARAM_STRING | PARAM_REQUIRED, storage, NULL, short_option, long_option);
}

static void regist_param_keyvalue_int_required(const char* short_option, const char* long_option, int* storage)
{
    regist_param_core(PARAM_KEYVALUE | PARAM_INT | PARAM_REQUIRED, NULL, storage, short_option, long_option);
}

static void regist_param_keyvalue_string(const char* short_option, const char* long_option, char** storage)
{
    regist_param_core(PARAM_KEYVALUE | PARAM_STRING, storage, NULL, short_option, long_option);
}

static void regist_param_keyvalue_int(const char* short_option, const char* long_option, int* storage)
{
    regist_param_core(PARAM_KEYVALUE | PARAM_INT, NULL, storage, short_option, long_option);
}

static void usage(const char* argv0)
{
    printf("usage: %s\n", argv0);
}

int main(int argc, char** argv)
{
    char* string_param = NULL;
    int int_param = 0;
    int int_param2 = 0;
    int help_flag = 0;
    int option1 = 0;

    regist_param_indexed_string_required(&string_param);
    regist_param_indexed_int_required(&int_param);
    regist_param_indexed_int(&int_param2);
    regist_param_flag("-h", "--help", &help_flag);
    regist_param_keyvalue_int("-o", "--option", &option1);

    if (parse_param(argc, argv) != 0)
    {
        usage(argv[0]);
        printf("rtn:-1\n");
        return -1;
    }
    if (help_flag == 1)
    {
        usage(argv[0]);
        printf("rtn:0\n");
        return 0;
    }

    printf("string_param: %s\n", string_param);
    printf("int_param: %d\n", int_param);
    printf("int_param2: %d\n", int_param2);
    printf("option1: %d\n", option1);

    return 0;
}
