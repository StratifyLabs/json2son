
#include <cstdio>
#include <sapi/sys.hpp>
#include <sapi/var.hpp>
#include <sapi/fmt.hpp>


static Printer p;

static int json2son(const String & input, const String & output);
static int json2son_recursive(const ConstString & key, const JsonValue & value, Son & output);
static int son2json(const String & input, const String & output);

int main(int argc, char * argv[]){
	Cli cli(argc, argv);
	cli.set_publisher("Stratify Labs, Inc");
	cli.handle_version();

	String input;
	String output;

	if( cli.is_option("-in") ){
		input = cli.get_option_argument("-in");
	} else {
		p.error("must specify -in <filename>");
		exit(1);
	}

	if( cli.is_option("-out") ){
		output = cli.get_option_argument("-out");
	} else {
		p.error("must specify -o <filename>");
		exit(1);
	}

	if( (input.find(".json") != String::npos) && (output.find(".son") != String::npos) ){
		if( json2son(input, output) < 0){
			//failed
			exit(1);
		}
	}

	if( (input.find(".son") != String::npos) && (output.find(".json") != String::npos) ){
		if( son2json(input, output) < 0){
			//failed
			exit(1);
		}
	}



	return 0;
}

int son2json(const String & input, const String & output){

	Son input_son;

	p.open_object("son2json");

	p.key("input", input);
	p.key("output", output);

	if( input_son.open_read(input) < 0 ){
		p.error("failed to open input file");
		p.close_object();
		return -1;
	}

	if( input_son.to_json(output) <  0 ){
		p.error("failed to convert input to json");
		p.close_object();
		return -1;
	}

	p.message("conversion completed successfully");
	p.close_object();
	return 0;
}

int json2son(const String & input, const String & output){
	JsonDocument json_document;
	JsonObject json_object;

	p.open_object("json2son");

	p.key("input", input);
	p.key("output", output);


	json_object = json_document.load_from_file(input).to_object();

	if( json_object.is_object() ){
		p.message("got valid json object");
	}

	Son output_son;

	if( output_son.create(output) < 0 ){
		p.error("failed to create output file");
		p.close_object();
		return -1;
	}

	if( json2son_recursive("", json_object, output_son) < 0 ){
		p.error("failed to convert output");
		p.close_object();
	}

	p.message("conversion completed successfully");

	p.close_object();

	return 0;

}

int json2son_recursive(const ConstString & key, const JsonValue & value, Son & output){

	switch(value.type()){
		case JsonValue::INVALID:
			p.error("key %s is invalid", key.str());
			return -1;
		case JsonValue::STRING: return output.write(key, value.to_string());
		case JsonValue::INTEGER: return output.write(key, (s32)value.to_integer());
		case JsonValue::TRUE: return output.write(key, true);
		case JsonValue::FALSE: return output.write(key, false);
		case JsonValue::REAL: return output.write(key, value.to_real());
		case JsonValue::ZERO: return output.write(key, ConstString());
		case JsonValue::OBJECT:
			if(1){
				Vector<String> keys = value.to_object().keys();
				output.open_object(key);
				for(u32 i=0; i < keys.count(); i++){
					if( json2son_recursive(keys.at(i), value.to_object().at(keys.at(i)), output) < 0 ){
						output.close_object();
						return -1;
					}
				}
				output.close_object();
			}
			return 0;

		case JsonValue::ARRAY:
			output.open_array(key);
			for(u32 i=0; i < value.to_array().count(); i++){
				if( json2son_recursive("", value.to_array().at(i), output) < 0 ){
					output.close_array();
					return -1;
				}
			}
			output.close_array();
			return 0;
	}

	p.error("type %d is not recognized", value.type());

	return -1;
}

