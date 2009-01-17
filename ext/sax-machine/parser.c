#include <search.h>
#include <string.h>
#include <stdio.h>
#include <ruby.h>
#include <stdlib.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlreader.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>

struct SAXMachineElement {
	const char * name;
	const char * method_name;
	const char * value;
};
struct SAXMachineHandler {
	SAXMachineElement elements[50];
};

const char * saxMachineTag;

/*
 * call-seq:
 *  parse_memory(data)
 *
 * Parse the document stored in +data+
 */
static VALUE parse_memory(VALUE self, VALUE data)
{
  xmlSAXHandlerPtr handler;
  Data_Get_Struct(self, xmlSAXHandler, handler);
  xmlSAXUserParseMemory(  handler,
                          (void *)self,
                          StringValuePtr(data),
                          NUM2INT(rb_funcall(data, rb_intern("length"), 0))
  );
  return data;
}

static void start_document(void * ctx)
{
  VALUE self = (VALUE)ctx;
  rb_funcall(self, rb_intern("start_document"), 0);
}

static void end_document(void * ctx)
{
  VALUE self = (VALUE)ctx;
  rb_funcall(self, rb_intern("end_document"), 0);
}

static void start_element(void * ctx, const xmlChar *name, const xmlChar **atts)
{
	if (strcmp((const char *)name, saxMachineTag) == 0) {
	  VALUE self = (VALUE)ctx;
	  VALUE attributes = rb_ary_new();
	  const xmlChar * attr;
	  int i = 0;
	  if(atts) {
	    while((attr = atts[i]) != NULL) {
	      rb_funcall(attributes, rb_intern("<<"), 1, rb_str_new2((const char *)attr));
	      i++;
	    }
	  }

	  rb_funcall( self,
	              rb_intern("start_element"),
	              2,
	              rb_str_new2((const char *)name),
	              attributes
	  );
	}
}

static void end_element(void * ctx, const xmlChar *name)
{
  VALUE self = (VALUE)ctx;
  rb_funcall(self, rb_intern("end_element"), 1, rb_str_new2((const char *)name));
}

static void characters_func(void * ctx, const xmlChar * ch, int len)
{
  VALUE self = (VALUE)ctx;
  VALUE str = rb_str_new((const char *)ch, (long)len);
  rb_funcall(self, rb_intern("characters"), 1, str);
}

static void comment_func(void * ctx, const xmlChar * value)
{
  VALUE self = (VALUE)ctx;
  VALUE str = rb_str_new2((const char *)value);
  rb_funcall(self, rb_intern("comment"), 1, str);
}

#ifndef XP_WIN
static void warning_func(void * ctx, const char *msg, ...)
{
  VALUE self = (VALUE)ctx;
  char * message;

  va_list args;
  va_start(args, msg);
  vasprintf(&message, msg, args);
  va_end(args);

  rb_funcall(self, rb_intern("warning"), 1, rb_str_new2(message));
  free(message);
}
#endif

#ifndef XP_WIN
static void error_func(void * ctx, const char *msg, ...)
{
  VALUE self = (VALUE)ctx;
  char * message;

  va_list args;
  va_start(args, msg);
  vasprintf(&message, msg, args);
  va_end(args);

  rb_funcall(self, rb_intern("error"), 1, rb_str_new2(message));
  free(message);
}
#endif

static void cdata_block(void * ctx, const xmlChar * value, int len)
{
  VALUE self = (VALUE)ctx;
  VALUE string = rb_str_new((const char *)value, (long)len);
  rb_funcall(self, rb_intern("cdata_block"), 1, string);
}

static void deallocate(xmlSAXHandlerPtr handler)
{
  free(handler);
}

static VALUE allocate(VALUE klass)
{
  xmlSAXHandlerPtr handler = calloc(1, sizeof(xmlSAXHandler));

  handler->startDocument = start_document;
  handler->endDocument = end_document;
  handler->startElement = start_element;
  handler->endElement = end_element;
  handler->characters = characters_func;
  handler->comment = comment_func;
#ifndef XP_WIN
  /*
   * The va*functions aren't in ming, and I don't want to deal with
   * it right now.....
   *
   */
  handler->warning = warning_func;
  handler->error = error_func;
#endif
  handler->cdataBlock = cdata_block;

  return Data_Wrap_Struct(klass, NULL, deallocate, handler);
}

static VALUE add_tag(VALUE self, VALUE tagName) {
	saxMachineTag = StringValuePtr(tagName);
	return tagName;
}

VALUE cNokogiriXmlSaxParser ;
void Init_native()
{
  VALUE mSAXMachine = rb_const_get(rb_cObject, rb_intern("SAXMachine"));
  VALUE klass = cNokogiriXmlSaxParser =
    rb_const_get(mSAXMachine, rb_intern("Parser"));
  rb_define_alloc_func(klass, allocate);
  rb_define_method(klass, "parse_memory", parse_memory, 1);
  rb_define_method(klass, "add_tag", add_tag, 1);
}
