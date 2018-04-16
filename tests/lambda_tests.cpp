#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "cafe_commands.h"
#include "lambda.h"
#include "lambdamu.h"
#include "Globals.h"
#include <gene_family.h>

extern "C" {
#include "cafe.h"
#include <cafe_shell.h>
	void cafe_shell_set_lambda(pCafeParam param, double* parameters);
	extern pBirthDeathCacheArray probability_cache;
    int __cafe_cmd_lambda_tree(pCafeParam param, char *arg1, char *arg2);
    extern struct chooseln_cache cache;
};

static void init_cafe_tree(Globals& globals)
{
	const char *newick_tree = "(((chimp:6,human:6):81,(mouse:17,rat:17):70):6,dog:9)";

	char buf[100];
	strcpy(buf, "tree ");
	strcat(buf, newick_tree);
	cafe_shell_dispatch_command(globals, buf);
}

int lambda_cmd_helper(Globals& globals)
{
	std::vector<std::string> strs;
	strs.push_back("lambda");
	strs.push_back("-s");
	strs.push_back("-t");
	strs.push_back("(((2,2)1,(1,1)1)1,1)");
	return cafe_cmd_lambda(globals, strs);
}


TEST_GROUP(LambdaTests)
{
};

TEST(LambdaTests, cafe_cmd_lambda_fails_without_tree)
{
	Globals globals;
	CafeFamily fam;
	globals.param.pfamily = &fam;

	try
	{ 
		lambda_cmd_helper(globals);
		FAIL("Expected exception not thrown");
	}
	catch (std::runtime_error& ex)
	{
		const char *expected = "ERROR: The tree was not loaded. Please load a tree with the 'tree' command.";
		STRCMP_CONTAINS(expected, ex.what());
	}
}

TEST(LambdaTests, cafe_cmd_lambda_fails_without_load)
{
	Globals globals;

	try
	{
		lambda_cmd_helper(globals);
		FAIL("Expected exception not thrown");
	}
	catch (std::runtime_error& ex)
	{
		const char *expected = "ERROR: The gene families were not loaded. Please load gene families with the 'load' command";
		STRCMP_CONTAINS(expected, ex.what());
	}
}

TEST(LambdaTests, PrepareCafeParam)
{
	Globals globals;
	CafeFamily fam;
	CafeTree tree;
	globals.param.pfamily = &fam;
	globals.param.pcafe = &tree;
	globals.param.lambda_tree = 0;
	globals.mu_tree = NULL;
	globals.Prepare();
	POINTERS_EQUAL(0, globals.param.lambda);
	POINTERS_EQUAL(0, globals.param.mu);
	LONGS_EQUAL(-1, globals.param.num_lambdas);
	LONGS_EQUAL(-1, globals.param.num_mus);
	LONGS_EQUAL(0, globals.param.parameterized_k_value);
	LONGS_EQUAL(LAMBDA_ONLY, globals.param.optimizer_init_type);
}

TEST(LambdaTests, TestCmdLambda)
{
	Globals globals;
	globals.param.quiet = 1;
	init_cafe_tree(globals);
	birthdeath_cache_init(2, &cache);

  globals.param.pfamily = cafe_family_init({"Dog", "Chimp", "Human", "Mouse", "Rat" });

  cafe_family_add_item(globals.param.pfamily, gene_family("ENSF00000002390", "OTOPETRIN", { 7, 7, 13, 18, 7 }));

	init_family_size(&globals.param.family_size, globals.param.pfamily->max_size);
	cafe_tree_set_parameters(globals.param.pcafe, &globals.param.family_size, 0);
	cafe_family_set_species_index(globals.param.pfamily, globals.param.pcafe);
	globals.param.ML = (double*)calloc(globals.param.pfamily->flist->size, sizeof(double));
	globals.param.MAP = (double*)calloc(globals.param.pfamily->flist->size, sizeof(double));

	LONGS_EQUAL(0, lambda_cmd_helper(globals));
  cafe_family_free(globals.param.pfamily);

};

TEST(LambdaTests, TestLambdaTree)
{
	Globals globals;
	init_cafe_tree(globals);
	char strs[2][100];
	strcpy(strs[0], "(((2,2)1,(1,1)1)1,1)");
    strs[1][0] = 0;

	Argument arg;
	arg.argc = 1;
	char* argv[] = { strs[0], strs[1] };
	arg.argv = argv;
	__cafe_cmd_lambda_tree(&globals.param, argv[0], argv[1]);
};

TEST(LambdaTests, Test_arguments)
{
	Globals globals;
    globals.param.quiet = 1;
	init_cafe_tree(globals);
	std::vector<std::string> strs;
	strs.push_back("lambda");
	strs.push_back("-t");
	strs.push_back("(((2,2)1,(1,1)1)1,1)");
	std::vector<Argument> pal = build_argument_list(strs);
	lambda_args args;
	args.load(pal);
	CHECK_FALSE(args.search);
	CHECK_FALSE(args.checkconv);
	CHECK_TRUE(args.lambda_tree != 0);
	LONGS_EQUAL(2, args.lambdas.size());
	DOUBLES_EQUAL(0, args.vlambda, .001);
	LONGS_EQUAL(UNDEFINED_LAMBDA, args.lambda_type);

	strs.push_back("-s");
	pal = build_argument_list(strs);
	args.load(pal);
	CHECK_TRUE(args.search);

	strs.push_back("-checkconv");
	pal = build_argument_list(strs);
	args.load(pal);
	CHECK_TRUE(args.checkconv);

	strs.push_back("-v");
	strs.push_back("14.6");
	pal = build_argument_list(strs);
	args.load(pal);
	DOUBLES_EQUAL(14.6, args.vlambda, .001);
	LONGS_EQUAL(SINGLE_LAMBDA, args.lambda_type);

	strs.push_back("-k");
	strs.push_back("19");
	pal = build_argument_list(strs);
	args.load(pal);
	LONGS_EQUAL(19, args.k_weights.size());

	strs.push_back("-f");
	pal = build_argument_list(strs);
	args.load(pal);
	LONGS_EQUAL(1, args.fixcluster0);
};


TEST(LambdaTests, Test_l_argument)
{
	Globals globals;
	init_cafe_tree(globals);
	std::vector<std::string> strs;
	strs.push_back("lambda");
	strs.push_back("-l");
	strs.push_back("15.6");
	strs.push_back("9.2");
	strs.push_back("21.8");
	std::vector<Argument> pal = build_argument_list(strs);
	lambda_args args;
	args.load(pal);
	LONGS_EQUAL(3, args.num_params);
	LONGS_EQUAL(MULTIPLE_LAMBDAS, args.lambda_type);
	DOUBLES_EQUAL(15.6, args.lambdas[0], .001);
	DOUBLES_EQUAL(9.2, args.lambdas[1], .001);
	DOUBLES_EQUAL(21.8, args.lambdas[2], .001);
};

TEST(LambdaTests, Test_p_argument)
{
	Globals globals;
	init_cafe_tree(globals);
	std::vector<std::string> strs;
	strs.push_back("lambda");
	strs.push_back("-p");
	strs.push_back("15.6");
	strs.push_back("9.2");
	strs.push_back("21.8");
	std::vector<Argument> pal = build_argument_list(strs);
	lambda_args args;
	args.load(pal);
	LONGS_EQUAL(3, args.num_params);
	DOUBLES_EQUAL(15.6, args.k_weights[0], .001);
	DOUBLES_EQUAL(9.2, args.k_weights[1], .001);
	DOUBLES_EQUAL(21.8, args.k_weights[2], .001);
};

TEST(LambdaTests, Test_r_argument)
{
	Globals globals;
	init_cafe_tree(globals);
	std::vector<std::string> strs;
	strs.push_back("lambda");
	strs.push_back("-r");
	strs.push_back("1:2:3");
	strs.push_back("-o");
	strs.push_back("test.txt");
	std::vector<Argument> pal = build_argument_list(strs);
	lambda_args args;
	args.load(pal);
	STRCMP_EQUAL("test.txt", args.outfile.c_str());
	LONGS_EQUAL(1, args.range.size());
	DOUBLES_EQUAL(1, args.range[0].start, .001);
	DOUBLES_EQUAL(2, args.range[0].step, .001);
	DOUBLES_EQUAL(3, args.range[0].end, .001);
};

TEST(LambdaTests, set_all_lambdas)
{
	CafeParam param;
	// shows that existing lambda values will be released
	param.lambda = (double *)memory_new(10, sizeof(double));
	param.num_lambdas = 15;
	set_all_lambdas(&param, 17.9);
	DOUBLES_EQUAL(17.9, param.lambda[0], 0.01);
	DOUBLES_EQUAL(17.9, param.lambda[10], 0.01);
	DOUBLES_EQUAL(17.9, param.lambda[14], 0.01);
};

TEST(LambdaTests, initialize_params_and_k_weights)
{
	CafeParam param;
	param.input.parameters = NULL;
	param.k_weights = NULL;
	param.num_params = 5;
	initialize_params_and_k_weights(&param, INIT_PARAMS);
	CHECK(param.input.parameters != NULL);
	CHECK(param.k_weights == NULL);

	param.input.parameters = NULL;
	param.parameterized_k_value = 5;
	initialize_params_and_k_weights(&param, INIT_KWEIGHTS);
	CHECK(param.input.parameters == NULL);
	CHECK(param.k_weights != NULL);
}

TEST(LambdaTests, set_parameters)
{
	lambda_args args;
	CafeParam param;

	double bar[] = { 6,7,8,9,10 };

	args.k_weights.resize(7);
	args.fixcluster0 = 3;
	args.lambdas.push_back(1);
	args.lambdas.push_back(2);
	copy(bar, bar + 5, args.k_weights.begin());
	args.num_params = 14;

	param.input.parameters = NULL;
	param.k_weights = NULL;
	param.num_lambdas = 1;

	set_parameters(&param, args);
	LONGS_EQUAL(7, param.parameterized_k_value);
	LONGS_EQUAL(14, param.num_params);
	DOUBLES_EQUAL(1.0, param.input.parameters[0], 0.001);
	DOUBLES_EQUAL(2.0, param.input.parameters[1], 0.001);
	DOUBLES_EQUAL(6.0, param.input.parameters[4], 0.001);
	DOUBLES_EQUAL(7.0, param.input.parameters[5], 0.001);
	DOUBLES_EQUAL(8.0, param.input.parameters[6], 0.001);
	DOUBLES_EQUAL(9.0, param.input.parameters[7], 0.001);
	DOUBLES_EQUAL(10.0, param.input.parameters[8], 0.001);
}

void mock_set_params(pCafeParam param, double* parameters)
{

}

TEST(LambdaTests, lambda_set)
{
	double bar[] = { 6,7,8,9,10 };

	lambda_args args;
	CafeParam param;
	args.k_weights.resize(7);
	args.fixcluster0 = 3;
	args.lambdas.push_back(1);
	args.lambdas.push_back(2);
	copy(bar, bar + 5, args.k_weights.begin());
	param.input.parameters = NULL;
	param.k_weights = NULL;
	param.num_lambdas = 1;
	param.optimizer_init_type = DO_NOTHING;
	args.num_params = 14;

	lambda_set(&param, args);
}

static pCafeTree create_tree()
{
	family_size_range range;
	range.min = 0;
	range.max = 5;
	range.root_min = 0;
	range.root_max = 5;
	const char *newick_tree = "(((2,2)1,(1,1)1)1,1)";
	char tree[100];
	strcpy(tree, newick_tree);
	return cafe_tree_new(tree, &range, 0, 0);
}

TEST(LambdaTests, lambda_args__validate_parameter_count)
{
	lambda_args args;
	args.num_params = 5;
	args.lambda_tree = (pTree)create_tree();
	args.k_weights.resize(11);
	try
	{
		args.validate_parameter_count(7);
		FAIL("Expected exception not thrown");
	}
	catch (std::runtime_error& ex)
	{
		STRCMP_CONTAINS("ERROR (lambda): The total number of parameters was not correct.\n", ex.what());
		STRCMP_CONTAINS("The total number of lambdas (-l) and proportions (-p) are 5", ex.what());
		STRCMP_CONTAINS("but 7 were expected\n", ex.what());
		STRCMP_CONTAINS("based on the tree (((2,2)1,(1,1)1)1,1) and the 11 clusters (-k).\n", ex.what());
	}

	try
	{
		args.k_weights.clear();
		args.validate_parameter_count(7);
		FAIL("Expected exception not thrown");
	}
	catch (std::runtime_error& ex)
	{
		STRCMP_CONTAINS("ERROR (lambda): The total number of parameters was not correct.\n", ex.what());
		STRCMP_CONTAINS("The total number of lambdas (-l) are 5", ex.what());
		STRCMP_CONTAINS("but 7 were expected\n", ex.what());
		STRCMP_CONTAINS("based on the tree (((2,2)1,(1,1)1)1,1)\n", ex.what());
	}

	try
	{
		args.k_weights.resize(11);
		args.lambda_tree = NULL;
		args.validate_parameter_count(7);
		FAIL("Expected exception not thrown");
	}
	catch (std::runtime_error& ex)
	{
		STRCMP_CONTAINS("ERROR (lambda): The total number of parameters was not correct.\n", ex.what());
		STRCMP_CONTAINS("The total number of lambdas (-l) and proportions (-p) are 5", ex.what());
		STRCMP_CONTAINS("but 7 were expected\n", ex.what());
	}

	try
	{
		args.k_weights.clear();
		args.validate_parameter_count(7);
		FAIL("Expected exception not thrown");
	}
	catch (std::runtime_error& ex)
	{
		STRCMP_CONTAINS("ERROR (lambda): The total number of parameters was not correct.\n", ex.what());
		STRCMP_CONTAINS("The total number of lambdas (-l) are 5", ex.what());
		STRCMP_CONTAINS("but 7 were expected\n", ex.what());
	}
}

TEST(LambdaTests, lambdamu_args__validate_parameter_count)
{
	lambdamu_args args;
	args.num_params = 5;
	args.lambda_tree = (pTree)create_tree();
	args.k_weights.resize(11);

	try
	{
		args.validate_parameter_count(7);
		FAIL("Expected exception not thrown");
	}
	catch (std::runtime_error& ex)
	{
		STRCMP_CONTAINS("ERROR (lambdamu): The total number of parameters was not correct.\n", ex.what());
		STRCMP_CONTAINS("The total number of lambdas (-l) and mus (-m) and proportions (-p) are 5", ex.what());
		STRCMP_CONTAINS("but 7 were expected\n", ex.what());
		STRCMP_CONTAINS("based on the tree (((2,2)1,(1,1)1)1,1) and the 11 clusters (-k).\n", ex.what());
	}

	try
	{
		args.k_weights.clear();
		args.validate_parameter_count(7);
		FAIL("Expected exception not thrown");
	}
	catch (std::runtime_error& ex)
	{
		STRCMP_CONTAINS("ERROR (lambdamu): The total number of parameters was not correct.\n", ex.what());
		STRCMP_CONTAINS("The total number of lambdas (-l) and mus (-m) are 5", ex.what());
		STRCMP_CONTAINS("but 7 were expected\n", ex.what());
		STRCMP_CONTAINS("based on the tree (((2,2)1,(1,1)1)1,1)\n", ex.what());
	}

	try
	{
		args.lambda_tree = NULL;
		args.k_weights.resize(11);
		args.validate_parameter_count(7);
		FAIL("Expected exception not thrown");
	}
	catch (std::runtime_error& ex)
	{
		STRCMP_CONTAINS("ERROR (lambdamu): The total number of parameters was not correct.\n", ex.what());
		STRCMP_CONTAINS("The total number of lambdas (-l) and mus (-m) and proportions (-p) are 5", ex.what());
		STRCMP_CONTAINS("but 7 were expected\n", ex.what());
	}

	try
	{
		args.k_weights.clear();
		args.validate_parameter_count(7);
		FAIL("Expected exception not thrown");
	}
	catch (std::runtime_error& ex)
	{
		STRCMP_CONTAINS("ERROR (lambdamu): The total number of parameters was not correct.\n", ex.what());
		STRCMP_CONTAINS("The total number of lambdas (-l) and mus (-m) are 5", ex.what());
		STRCMP_CONTAINS("but 7 were expected\n", ex.what());
	}
}

TEST(LambdaTests, lambdamu_set)
{
	lambdamu_args args;
	args.lambdas.push_back(.1);
	args.lambdas.push_back(.2);
	args.mus.push_back(.3);
	args.mus.push_back(.4);
	args.num_params = 4;
	args.eqbg = 0;
	args.lambda_tree = (pTree)create_tree();

	CafeParam param;
	param.input.parameters = NULL;
	param.k_weights = NULL;
	param.num_lambdas = 2;
	param.optimizer_init_type = DO_NOTHING;
	lambdamu_set(&param, args);
	LONGS_EQUAL(4, param.num_params);
	DOUBLES_EQUAL(.1, param.input.parameters[0], .001);
	DOUBLES_EQUAL(.2, param.input.parameters[1], .001);
	DOUBLES_EQUAL(.3, param.input.parameters[2], .001);
	DOUBLES_EQUAL(.4, param.input.parameters[3], .001);
}

TEST(LambdaTests, lambdamu_set_without_lambda_tree)
{
	lambdamu_args args;
	args.lambdas.push_back(.1);
	args.lambdas.push_back(.2);
	args.mus.push_back(.3);
	args.mus.push_back(.4);
	args.num_params = 2;
	args.eqbg = 0;
	args.lambda_tree = NULL;

	CafeParam param;
	param.input.parameters = NULL;
	param.k_weights = NULL;
	param.num_lambdas = 2;
    param.optimizer_init_type = DO_NOTHING;
    lambdamu_set(&param, args);
	LONGS_EQUAL(2, param.num_params);
	DOUBLES_EQUAL(.1, param.input.parameters[0], .001);
	DOUBLES_EQUAL(.3, param.input.parameters[1], .001);
}

TEST(LambdaTests, lambdamu_set_with_k_weights)
{
	lambdamu_args args;
	args.lambdas.push_back(.1);
	args.mus.push_back(.3);
	args.k_weights.push_back(.5);
	args.k_weights.push_back(.6);

	args.num_params = 5;
	args.eqbg = 0;
	args.fixcluster0 = 0;
	args.lambda_tree = NULL;

	CafeParam param;
	param.input.parameters = NULL;
	param.k_weights = NULL;
	param.num_lambdas = 2;
    param.optimizer_init_type = DO_NOTHING;
    lambdamu_set(&param, args);
	LONGS_EQUAL(5, param.num_params);
	DOUBLES_EQUAL(.1, param.input.parameters[0], .001);
	DOUBLES_EQUAL(.3, param.input.parameters[2], .001);
	DOUBLES_EQUAL(.5, param.input.parameters[4], .001);
}

TEST(LambdaTests, lambdamu_set_with_k_weights_and_tree)
{
	lambdamu_args args;
	args.lambdas.push_back(.1);
	args.lambdas.push_back(.2);
	args.mus.push_back(.3);
	args.mus.push_back(.4);
	args.k_weights.push_back(.5);
	args.k_weights.push_back(.6);

	args.num_params = 9;
	args.eqbg = 0;
	args.fixcluster0 = 0;
	args.lambda_tree = (pTree)create_tree();

	Globals globals;
	globals.param.num_lambdas = 2;
	lambdamu_set(&globals.param, args);
	LONGS_EQUAL(9, globals.param.num_params);
	DOUBLES_EQUAL(.1, globals.param.input.parameters[0], .001);
	DOUBLES_EQUAL(.2, globals.param.input.parameters[1], .001);

	DOUBLES_EQUAL(.3, globals.param.input.parameters[4], .001);
	DOUBLES_EQUAL(.4, globals.param.input.parameters[5], .001);

	DOUBLES_EQUAL(.5, globals.param.input.parameters[8], .001);
}

TEST(LambdaTests, best_lambda_by_fminsearch)
{
	std::ostringstream ost;
	Globals globals;
	
	family_size_range range;
	range.min = 0;
	range.max = 5;
	range.root_min = 0;
	range.root_max = 5;
	const char *newick_tree = "((A:1,B:1):1,(C:1,D:1):1);";
	char tree[100];
	strcpy(tree, newick_tree);

    globals.param.quiet = 1;
	globals.param.pcafe = cafe_tree_new(tree, &range, 0, 0);
	globals.param.pfamily = cafe_family_init({ "A", "B", "C", "D" });

	cafe_family_add_item(globals.param.pfamily, gene_family("ENS01", "1", { 5, 10, 2, 6 }));
	cafe_family_add_item(globals.param.pfamily, gene_family("ENS02", "2", { 5, 10, 2, 6 }));
	cafe_family_add_item(globals.param.pfamily, gene_family("ENS03", "3", { 5, 10, 2, 6 }));
	cafe_family_add_item(globals.param.pfamily, gene_family("ENS04", "4", { 5, 10, 2, 6 }));

	init_family_size(&globals.param.family_size, globals.param.pfamily->max_size);
	cafe_tree_set_parameters(globals.param.pcafe, &globals.param.family_size, 0);
	cafe_family_set_species_index(globals.param.pfamily, globals.param.pcafe);

    std::vector<double> prior_rfsize;
	cafe_set_prior_rfsize_empirical(&globals.param, prior_rfsize);
    globals.param.prior_rfsize = (double *)memory_new(prior_rfsize.size(), sizeof(double));
    std::copy(prior_rfsize.begin(), prior_rfsize.end(), globals.param.prior_rfsize);
	globals.param.ML = (double*)calloc(globals.param.pfamily->flist->size, sizeof(double));
	globals.param.MAP = (double*)calloc(globals.param.pfamily->flist->size, sizeof(double));

	double x[] = { 0.05, 0.01 };
	globals.param.input.parameters = x;
    globals.param.num_params = 2;
    cafe_best_lambda_by_fminsearch(&globals.param, 2, 0);
	cafe_family_free(globals.param.pfamily);
}

TEST(LambdaTests, best_lambda_mu_by_fminsearch)
{
	std::ostringstream ost;
	Globals globals;
#if 0	// TODO
	best_lambda_mu_by_fminsearch(&globals.param, 1, 1, 1, ost);
#endif
}

static void set_matrix(pTree ptree, pTreeNode ptnode, va_list ap1)
{
	va_list ap;
	va_copy(ap, ap1);
	square_matrix* m = va_arg(ap, square_matrix*);
	pCafeNode pcnode = (pCafeNode)ptnode;
	pcnode->birthdeath_matrix = m;
	va_end(ap);
}

TEST(LambdaTests, get_posterior)
{
	probability_cache = NULL;

	family_size_range range;
	range.min = range.root_min = 0;
	range.max = range.root_max = 15;

	CafeParam param;
	param.flog = stdout;
	param.quiet = 1;
	param.prior_rfsize = NULL;

	const char *newick_tree = "(((chimp:6,human:6):81,(mouse:17,rat:17):70):6,dog:9)";
	char tree[100];
	strcpy(tree, newick_tree);
	param.pcafe = cafe_tree_new(tree, &range, 0.01, 0);
	LONGS_EQUAL(16, param.pcafe->size_of_factor);	// as a side effect of create_tree

	param.pfamily = cafe_family_init({ "chimp", "human", "mouse", "rat", "dog" });
	cafe_family_set_species_index(param.pfamily, param.pcafe);
	cafe_family_add_item(param.pfamily, gene_family("id", "description", { 3, 5, 7, 11, 13 }));

    std::vector<double> ml(15), map(15);

	square_matrix m;
	square_matrix_init(&m, 16);
	for (int i = 0; i < 256; i++)
		m.values[i] = .25;

	tree_traveral_postfix((pTree)param.pcafe, set_matrix, &m);

    std::vector<double> prior_rfsize;
    cafe_set_prior_rfsize_empirical(&param, prior_rfsize);
	DOUBLES_EQUAL(-4.6503, get_posterior(param.pfamily, param.pcafe, &param.family_size, ml, map, prior_rfsize, param.quiet), .0001);

	for (int i = 0; i < 256; i++)
		m.values[i] = 0;

	CHECK_THROWS(std::runtime_error, get_posterior(param.pfamily, param.pcafe, &param.family_size, ml, map, prior_rfsize, param.quiet));

	cafe_family_free(param.pfamily);
};

TEST(LambdaTests, find_poisson_lambda)
{
    CafeParam param;
    param.quiet = 0;
    param.flog = stdout;
    param.prior_rfsize = NULL;
    param.pfamily = cafe_family_init({ "A", "B", "C", "D" });
    cafe_family_add_item(param.pfamily, gene_family("ENS01", "description", { 6, 11, 3, 7 }));
    cafe_family_add_item(param.pfamily, gene_family("ENS02", "description", { 6, 11, 3, 7 }));
    cafe_family_add_item(param.pfamily, gene_family("ENS03", "description", { 6, 11, 3, 7 }));
    cafe_family_add_item(param.pfamily, gene_family("ENS04", "description", { 6, 11, 3, 7 }));

    const char *newick_tree = "((A:1,B:1):1,(C:1,D:1):1);";
    char tree[100];
    strcpy(tree, newick_tree);
    family_size_range range;
    range.min = 0;
    range.max = 7;
    range.root_min = 0;
    range.root_max = 7;
    param.pcafe = cafe_tree_new(tree, &range, 0, 0);

    cafe_family_set_species_index(param.pfamily, param.pcafe);

    poisson_lambda pl = find_poisson_lambda(param.pfamily);
    LONGS_EQUAL(1, pl.num_params);
    DOUBLES_EQUAL(5.75, pl.parameters[0], .001);
    free(pl.parameters);
    cafe_family_free(param.pfamily);
}

TEST(LambdaTests, get_posterior2)
{
    srand(10);

    if (cache.size > 0)
        chooseln_cache_free2(&cache);
    chooseln_cache_init2(&cache, 150);

    probability_cache = birthdeath_cache_init(150, &cache);

    family_size_range range;
    range.min = 0;
    range.root_min = 1;
    range.max = 149;
    range.root_max = 124;

    const char *newick_tree = "(A:1,B:1)";
    char tree[1000];
    strcpy(tree, newick_tree);
    double lambda = 0.27290862102823;
    double mu = -1;
    pCafeTree pcafe = cafe_tree_new(tree, &range, lambda, mu);

    pCafeFamily pfamily = cafe_family_init({ "A", "B" });
    cafe_family_add_item(pfamily, gene_family("ENS01", "description", { 1, 2 }));
    cafe_family_add_item(pfamily, gene_family("ENS02", "description", { 2, 1 }));
    cafe_family_add_item(pfamily, gene_family("ENS03", "description", { 3, 6 }));
    cafe_family_add_item(pfamily, gene_family("ENS04", "description", { 6, 3 }));
    cafe_family_set_species_index(pfamily, pcafe);

    std::vector<double> ml(pfamily->flist->size);
    std::vector<double> map(pfamily->flist->size);
    std::vector<double> rf(FAMILYSIZEMAX);
    double poisson_lambda = 2.0;
    cafe_set_prior_rfsize_poisson_lambda(rf, 1, &poisson_lambda);

    for (int i = 0; i < pcafe->super.nlist->size; ++i)
    {
        pCafeNode node = (pCafeNode)pcafe->super.nlist->array[i];
        node->birth_death_probabilities.lambda = lambda;
        node->birth_death_probabilities.mu =mu;
    }

    DOUBLES_EQUAL(-18.0085, get_posterior(pfamily, pcafe, &range, ml, map, rf, 1), .1);

    cafe_family_free(pfamily);
};

TEST(LambdaTests, collect_leaf_sizes)
{
    pCafeFamily pfamily = cafe_family_init({ "A", "B" });
    cafe_family_add_item(pfamily, gene_family("ENS01", "description", { 1, 2 }));
    cafe_family_add_item(pfamily, gene_family("ENS02", "description", { 2, 1 }));
    cafe_family_add_item(pfamily, gene_family("ENS03", "description", { 3, 6 }));
    cafe_family_add_item(pfamily, gene_family("ENS04", "description", { 6, 3 }));

    std::fill(pfamily->index, pfamily->index + 2, 0);

    auto pal = collect_leaf_sizes(pfamily);
    LONGS_EQUAL(8, pal.size());
    LONGS_EQUAL(0, pal[0]);
    LONGS_EQUAL(1, pal[1]);

    pfamily->index[1] = -1;
    auto pal2 = collect_leaf_sizes(pfamily);
    LONGS_EQUAL(4, pal2.size());
    LONGS_EQUAL(2, pal2[2]);
    LONGS_EQUAL(5, pal2[3]);

    cafe_family_free(pfamily);
}
